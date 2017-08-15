#include "serval.h"
#include "dataformats.h"
#include "conf.h"
#include "httpd.h"
#include "str.h"
#include "numeric_str.h"
#include "base64.h"
#include "strbuf_helpers.h"
#include "meshmb.h"

DEFINE_FEATURE(http_rest_meshmb);

static char *PART_MESSAGE = "message";
static int send_part_start(struct http_request *hr)
{
  httpd_request *r = (httpd_request *) hr;
  assert(r->u.sendmsg.current_part == NULL);
  return 0;
}

static int send_part_end(struct http_request *hr)
{
  httpd_request *r = (httpd_request *) hr;
  if (r->u.sendmsg.current_part == PART_MESSAGE) {
    if (r->u.sendmsg.message.length == 0)
      return http_response_form_part(r, 400, "Invalid (empty)", PART_MESSAGE, NULL, 0);
    r->u.sendmsg.received_message = 1;
    DEBUGF(httpd, "received %s = %s", PART_MESSAGE, alloca_toprint(-1, r->u.sendmsg.message.buffer, r->u.sendmsg.message.length));
  } else
    FATALF("current_part = %s", alloca_str_toprint(r->u.sendmsg.current_part));
  r->u.sendmsg.current_part = NULL;
  return 0;
}

static int send_part_header(struct http_request *hr, const struct mime_part_headers *h)
{
  httpd_request *r = (httpd_request *) hr;
  if (!h->content_disposition.type[0])
    return http_response_content_disposition(r, 415, "Missing", h->content_disposition.type);
  if (strcmp(h->content_disposition.type, "form-data") != 0)
    return http_response_content_disposition(r, 415, "Unsupported", h->content_disposition.type);
  if (strcmp(h->content_disposition.name, PART_MESSAGE) == 0) {
    if (r->u.sendmsg.received_message)
      return http_response_form_part(r, 400, "Duplicate", PART_MESSAGE, NULL, 0);
    r->u.sendmsg.current_part = PART_MESSAGE;
    form_buf_malloc_init(&r->u.sendmsg.message, MESSAGE_PLY_MAX_LEN);
  }
  else
    return http_response_form_part(r, 415, "Unsupported", h->content_disposition.name, NULL, 0);
  if (!h->content_type.type[0] || !h->content_type.subtype[0])
    return http_response_content_type(r, 400, "Missing", &h->content_type);
  if (strcmp(h->content_type.type, "text") != 0 || strcmp(h->content_type.subtype, "plain") != 0)
    return http_response_content_type(r, 415, "Unsupported", &h->content_type);
  if (!h->content_type.charset[0])
    return http_response_content_type(r, 400, "Missing charset", &h->content_type);
  if (strcmp(h->content_type.charset, "utf-8") != 0)
    return http_response_content_type(r, 415, "Unsupported charset", &h->content_type);
  return 0;
}

static int send_part_body(struct http_request *hr, char *buf, size_t len)
{
  httpd_request *r = (httpd_request *) hr;
  if (r->u.sendmsg.current_part == PART_MESSAGE) {
    form_buf_malloc_accumulate(r, PART_MESSAGE, &r->u.sendmsg.message, buf, len);
  } else
    FATALF("current_part = %s", alloca_str_toprint(r->u.sendmsg.current_part));
  return 0;
}

static int send_content_end(struct http_request *hr)
{
  httpd_request *r = (httpd_request *) hr;
  if (!r->u.sendmsg.received_message)
    return http_response_form_part(r, 400, "Missing", PART_MESSAGE, NULL, 0);
  assert(r->u.sendmsg.message.length > 0);
  assert(r->u.sendmsg.message.length <= MESSAGE_PLY_MAX_LEN);

  keyring_identity *id = keyring_find_identity(keyring, &r->bid);
  if (!id){
    http_request_simple_response(&r->http, 500, "TODO, detailed errors");
    return 500;
  }
  if (meshmb_send(id, r->u.sendmsg.message.buffer, r->u.sendmsg.message.length, 0, NULL)==-1){
    http_request_simple_response(&r->http, 500, "TODO, detailed errors");
    return 500;
  }
  http_request_simple_response(&r->http, 201, "TODO, detailed response");
  return 201;
}

static void send_finalise(httpd_request *r)
{
  form_buf_malloc_release(&r->u.sendmsg.message);
}

static int restful_meshmb_send(httpd_request *r, const char *remainder)
{
  if (*remainder)
    return 404;
  assert(r->finalise_union == NULL);
  r->finalise_union = send_finalise;
  // Parse the request body as multipart/form-data.
  assert(r->u.sendmsg.current_part == NULL);
  assert(!r->u.sendmsg.received_message);
  r->http.form_data.handle_mime_part_start = send_part_start;
  r->http.form_data.handle_mime_part_end = send_part_end;
  r->http.form_data.handle_mime_part_header = send_part_header;
  r->http.form_data.handle_mime_body = send_part_body;
  // Send the message once the body has arrived.
  r->http.handle_content_end = send_content_end;
  return 1;
}

static strbuf position_token_to_str(strbuf b, uint64_t position)
{
  uint8_t tmp[12];
  char tmp_str[BASE64_ENCODED_LEN(12)+1];

  int len = pack_uint(tmp, position);
  assert(len <= (int)sizeof tmp);
  size_t n = base64url_encode(tmp_str, tmp, len);
  tmp_str[n] = '\0';
  return strbuf_puts(b, tmp_str);
}

static int strn_to_position_token(const char *str, uint64_t *position, const char **afterp)
{
  uint8_t token[12];
  size_t token_len = base64url_decode(token, sizeof token, str, 0, afterp, 0, NULL);

  int unpacked;
  if ((unpacked = unpack_uint(token, token_len, position))!=-1
    && **afterp=='/'){
    (*afterp)++;
  } else {
    *position = 0;
    *afterp=str;
  }
  return 1;
}

static int next_ply_message(httpd_request *r){
  if (!message_ply_is_open(&r->u.plylist.ply_reader)){
    if (message_ply_read_open(&r->u.plylist.ply_reader, &r->bid)==-1){
      r->u.plylist.eof = 1;
      return -1;
    }

    // skip back to where we were
    if (r->u.plylist.current_offset)
      r->u.plylist.ply_reader.read.offset = r->u.plylist.current_offset;

    DEBUGF(httpd, "Opened ply @%"PRIu64, r->u.plylist.ply_reader.read.offset);
  }

  if (r->u.plylist.current_offset==0){
    // enumerate everything from the top
    DEBUGF(httpd, "Started reading @%"PRIu64, r->u.plylist.ply_reader.read.length);
    r->u.plylist.current_offset =
    r->u.plylist.start_offset =
    r->u.plylist.ply_reader.read.offset =
      r->u.plylist.ply_reader.read.length;
  }

  while(message_ply_read_prev(&r->u.plylist.ply_reader) == 0){
    r->u.plylist.current_offset = r->u.plylist.ply_reader.record_end_offset;
    if (r->u.plylist.current_offset <= r->u.plylist.end_offset){
      DEBUGF(httpd, "Hit end %"PRIu64" @%"PRIu64,
	r->u.plylist.end_offset, r->u.plylist.current_offset);
      break;
    }

    switch(r->u.plylist.ply_reader.type){
      case MESSAGE_BLOCK_TYPE_TIME:
	if (r->u.plylist.ply_reader.record_length<4){
	  WARN("Malformed ply, expected 4 byte timestamp");
	  continue;
	}
	r->u.plylist.timestamp = read_uint32(r->u.plylist.ply_reader.record);
	break;

      case MESSAGE_BLOCK_TYPE_MESSAGE:
	r->u.plylist.eof = 0;
	return 1;

      case MESSAGE_BLOCK_TYPE_ACK:
	// TODO, link to some other ply?
	break;

      default:
	//ignore unknown types
	break;
    }
  }
  r->u.plylist.eof = 1;
  return 0;
}

static int restful_meshmb_list_json_content_chunk(struct http_request *hr, strbuf b)
{
  httpd_request *r = (httpd_request *) hr;
  // The "my_sid" and "their_sid" per-conversation fields allow the same JSON structure to be used
  // in a future, non-SID-specific request, eg, to list all conversations for all currently open
  // identities.
  const char *headers[] = {
    "offset",
    "token",
    "text",
    "timestamp"
  };

  DEBUGF(httpd, "Phase %d", r->u.plylist.phase);

  switch (r->u.plylist.phase) {
    case LIST_HEADER:

      strbuf_puts(b, "{\n");

      // open the ply now in order to read the manifest name
      if (!message_ply_is_open(&r->u.plylist.ply_reader))
	next_ply_message(r);

      if (r->u.plylist.ply_reader.name)
	strbuf_sprintf(b, "\"name\":\"%s\",\n", r->u.plylist.ply_reader.name);

      strbuf_puts(b, "\"header\":[");
      unsigned i;
      for (i = 0; i != NELS(headers); ++i) {
	if (i)
	  strbuf_putc(b, ',');
	strbuf_json_string(b, headers[i]);
      }
      strbuf_puts(b, "],\n\"rows\":[");
      if (!strbuf_overrun(b))
	r->u.plylist.phase = LIST_ROWS;
      return 1;

ROWS:
    case LIST_ROWS:
    case LIST_FIRST:

      if (!message_ply_is_open(&r->u.plylist.ply_reader)){
	// re-load the current message text
	if (next_ply_message(r)!=1)
	  goto END;
      } else if (r->u.plylist.eof)
	  goto END;

      if (r->u.plylist.rowcount!=0)
	strbuf_putc(b, ',');
      strbuf_puts(b, "\n[");

      strbuf_sprintf(b, "%"PRIu64, r->u.plylist.current_offset);
      strbuf_puts(b, ",\"");
      position_token_to_str(b, r->u.plylist.current_offset);
      strbuf_puts(b, "\",");
      strbuf_json_string(b, (const char *)r->u.plylist.ply_reader.record);
      strbuf_putc(b, ',');
      strbuf_sprintf(b, "%d", r->u.plylist.timestamp);
      strbuf_puts(b, "]");

      if (!strbuf_overrun(b)) {
	++r->u.plylist.rowcount;
	if (next_ply_message(r)!=1)
	  r->u.plylist.phase = LIST_END;
      }
      return 1;

END:
      r->u.plylist.phase = LIST_END;
    case LIST_END:

      {
	time_ms_t now;
	// during a new-since request, we don't really want to end until the time limit has elapsed
	if (r->u.plylist.end_time && (now = gettime_ms()) < r->u.plylist.end_time) {
	  // where we started this time, will become where we end on the next pass;
	  r->u.plylist.end_offset = r->u.plylist.start_offset;
	  r->u.plylist.current_offset = 0;
	  r->u.plylist.phase = LIST_ROWS;

	  if (r->u.plylist.ply_reader.read.length > r->u.plylist.start_offset && next_ply_message(r)==1)
	    // new content arrived while we were iterating, we can resume immediately
	    goto ROWS;

	  message_ply_read_close(&r->u.plylist.ply_reader);
	  http_request_pause_response(&r->http, r->u.plylist.end_time);
	  return 0;
	}
      }

      strbuf_puts(b, "\n]\n}\n");
      if (!strbuf_overrun(b))
	r->u.plylist.phase = LIST_DONE;
      // fall through...
    case LIST_DONE:
      return 0;
  }
  abort();
  return 0;
}

static int restful_meshmb_list_json_content(struct http_request *hr, unsigned char *buf, size_t bufsz, struct http_content_generator_result *result)
{
  return generate_http_content_from_strbuf_chunks(hr, (char *)buf, bufsz, result, restful_meshmb_list_json_content_chunk);
}

static void list_on_rhizome_add(httpd_request *r, rhizome_manifest *m)
{
  if (strcmp(m->service, RHIZOME_SERVICE_MESHMB) == 0
    && cmp_rhizome_bid_t(&m->keypair.public_key, &r->bid)==0) {
    message_ply_read_close(&r->u.plylist.ply_reader);
    http_request_resume_response(&r->http);
  }
}

static void list_finalise(httpd_request *r)
{
  message_ply_read_close(&r->u.plylist.ply_reader);
}

static int restful_meshmb_list(httpd_request *r, const char *remainder)
{
  if (*remainder)
    return 404;
  assert(r->finalise_union == NULL);
  r->finalise_union = list_finalise;
  r->trigger_rhizome_bundle_added = list_on_rhizome_add;
  r->u.plylist.phase = LIST_HEADER;
  r->u.plylist.rowcount = 0;
  r->u.plylist.end_offset = r->ui64;

  http_request_response_generated(&r->http, 200, CONTENT_TYPE_JSON, restful_meshmb_list_json_content);
  return 1;
}

static int restful_meshmb_newsince_list(httpd_request *r, const char *remainder)
{
  int ret;
  if ((ret = restful_meshmb_list(r, remainder))==1){
    r->u.plylist.end_time = gettime_ms() + config.api.restful.newsince_timeout * 1000;
  }
  return ret;
}

DECLARE_HANDLER("/restful/meshmb/", restful_meshmb_);
static int restful_meshmb_(httpd_request *r, const char *remainder)
{
  r->http.response.header.content_type = CONTENT_TYPE_JSON;
  if (!is_rhizome_http_enabled())
    return 404;
  int ret = authorize_restful(&r->http);
  if (ret)
    return ret;
  const char *verb = HTTP_VERB_GET;
  HTTP_HANDLER *handler = NULL;
  const char *end;

  if (strn_to_identity_t(&r->bid, remainder, &end) != -1) {
    remainder = end;

    if (strcmp(remainder, "/sendmessage") == 0) {
      handler = restful_meshmb_send;
      verb = HTTP_VERB_POST;
      remainder = "";
    } else if (strcmp(remainder, "/messagelist.json") == 0) {
      handler = restful_meshmb_list;
      remainder = "";
      r->ui64 = 0;
    } else if (   str_startswith(remainder, "/newsince/", &end)
	       && strn_to_position_token(end, &r->ui64, &end)
	       && strcmp(end, "messagelist.json") == 0) {
      handler = restful_meshmb_newsince_list;
      remainder = "";
    }
  }

  if (handler == NULL)
    return 404;
  if (r->http.verb != verb)
    return 405;
  return handler(r, remainder);
}
