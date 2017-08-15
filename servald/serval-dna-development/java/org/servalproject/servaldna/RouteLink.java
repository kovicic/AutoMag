package org.servalproject.servaldna;

import java.nio.BufferUnderflowException;
import java.nio.ByteBuffer;

/**
 * Created by jeremy on 10/05/16.
 */
public class RouteLink {

    private static final int REACHABLE_SELF = (1<<0);
    private static final int REACHABLE_BROADCAST = (1<<1);
    private static final int REACHABLE_UNICAST = (1<<2);
    //private static final int REACHABLE_INDIRECT = (1<<3);

    public final Subscriber subscriber;
    @Deprecated
    public final SubscriberId sid;

    public final SubscriberId next_hop;
    public final SubscriberId prior_hop;
    public final int hop_count;
    public final int interface_id;
    public final boolean interface_up;
    public final String interface_name;
    private final int reachable;

    public boolean isReachable(){
        return reachable!=0;
    }

    public boolean isNeighbour(){
        return (reachable&(REACHABLE_BROADCAST|REACHABLE_UNICAST))!=0;
    }

    public boolean isSelf() {
        return reachable == REACHABLE_SELF;
    }

    RouteLink(ByteBuffer buff) throws AbstractId.InvalidBinaryException, BufferUnderflowException {
        this.subscriber = new Subscriber(buff);
        this.sid = subscriber.sid;
        reachable = 0xFF & (int)buff.get();
        int hop_count=-1;
        SubscriberId next_hop = null;
        SubscriberId prior_hop = null;
        int interface_id=-1;
        String interface_name = null;
        boolean up = false;

        if (buff.hasRemaining()) {
            hop_count = 0xFF & (int)buff.get();
            if (hop_count>1) {
                next_hop = new SubscriberId(buff);
                if (hop_count>2)
                    prior_hop = new SubscriberId(buff);
            }else{
                interface_id = 0xFF & (int)buff.get();
                up = buff.get() != 0;
                StringBuilder builder = new StringBuilder();
                while(true){
                    byte b = buff.get();
                    if (b==0)
                        break;
                    builder.append((char)b);
                }
                interface_name = builder.toString();
            }
        }
        this.next_hop = next_hop;
        this.prior_hop = prior_hop;
        this.hop_count = hop_count;
        this.interface_id = interface_id;
        this.interface_up = up;
        this.interface_name = interface_name;
    }

    @Override
    public String toString() {
        return "RouteLink{" +
                "subscriber=" + subscriber +
                ", next_hop=" + next_hop +
                ", prior_hop=" + prior_hop +
                ", hop_count=" + hop_count +
                ", interface_id=" + interface_id +
                ", interface_name='" + interface_name + '\'' +
                ", reachable=" + reachable +
                '}';
    }

}
