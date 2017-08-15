	//N.B. Kod nije kompletan, dati su samo delovi

	uint8_t exitTriger, finishTriger, listTriger; 
	exitTriger = finishTriger = 1;
	int readChar, readInt, readType, readDef;

int mygetch(void) {
	struct termios oldt,
	               newt;
	int            ch;
	tcgetattr( STDIN_FILENO, &oldt );
	newt = oldt;
	newt.c_lflag &= ~( ICANON | ECHO );
	tcsetattr( STDIN_FILENO, TCSANOW, &newt );
	ch = getchar();
	tcsetattr( STDIN_FILENO, TCSANOW, &oldt );
	return ch;
}

	// Primer menija
	while(exitTriger){
		// program menu:
		system_clear_screen();
		printf ("****************************************\n");
		printf ("Choose your option :\n\n");
		printf ("S - Configure SBG\n");
		printf ("Z - ZigBee ping\n");
		printf ("D - STrackU config\n");			
		printf ("Q - Quit\n");
		printf ("****************************************\n\n");
		
		readType = mygetch();
		
		switch (readType){
			case 'S':
			case 'Z':
			case 'D':
				break;
			case 'Q':
				printf("Quiting...\n");
				exitTriger = 0;
				continue;
				break;				
			default:
				system_clear_screen();
				printf("Wrong option. Try again...\n");
				usleep(POLL_K_mSEC_5);
				continue;
				break;			
		}
	}

	//Primer ucitavanja parametara za jednu od opcija
	case 'S':
	memset(&sbg_config, 0, sizeof(STrack_sbg_t));
	sbg_config.motion_profile_id 	= 2;
	sbg_config.axis_dir_x 			= 0;
	sbg_config.axis_dir_y 			= 3;
	sbg_config.lever_x				= 0.0;
	sbg_config.lever_y				= 0.0;
	sbg_config.lever_z				= 0.0;

	sbg_config.target_baud 			= 38400;

	printf("[OTA] Default sbg params pre-set for node_num = %d\n", node_num);

	sbg_config.id_device |= (uint64_t)(nd_list[node_num])->addr_sh<<32;
	sbg_config.id_device |= (uint64_t)(nd_list[node_num])->addr_sl;

	unique_config.id_device = sbg_config.id_device;

	printf("Do you want to use it? (y/n)\n");
	int param_default = -1;
	while(param_default == -1){
		readDef = mygetch();

		switch(readDef){
			case 'y': param_default = 1;
				break;
			case 'n': param_default = 0;
				break;
			default:
				break;
		}
		if (param_default == -1)
		{
			printf("Wrong option: select y/n...\n");
		}
	}

	if(!param_default){
		posix_input_true = 0;
		while(!posix_input_true){
			printf("Enter SBG motion profile id(0,1,2)\n");
			scanf("%d", &(sbg_config.motion_profile_id));
			if (sbg_config.motion_profile_id <= 2) posix_input_true = 1;
			else{
				system_clear_screen();
				printf("Error. Try entering a valid SBG motion profile ID.\n");
			}																
		}

		posix_input_true = 0;
		while(!posix_input_true){							
			printf("Enter SBG X direction (0..5)[f,b,l,r,u,d]\n");
			scanf("%d", &(sbg_config.axis_dir_x));
			if (sbg_config.axis_dir_x <= 5) posix_input_true = 1;
			else{
				system_clear_screen();
				printf("Error. Try entering a valid SBG X direction.\n");
			}																
		}

		posix_input_true = 0;
		while(!posix_input_true){							
			printf("Enter SBG Y direction (0..5)[f,b,l,r,u,d]\n");
			scanf("%d", &(sbg_config.axis_dir_y));
			if (sbg_config.axis_dir_y <= 5) posix_input_true = 1;
			else{
				system_clear_screen();
				printf("Error. Try entering a valid SBG Y direction.\n");
			}																
		}

		posix_input_true = 0;
		while(!posix_input_true){							
			printf("Enter SBG target baudrate\n");
			scanf("%d", &(sbg_config.target_baud));
			if (sbg_config.target_baud <= 115200) posix_input_true = 1;
			else{
				system_clear_screen();
				printf("Error. Try entering a valid SBG baudrate.\n");
			}																
		}
		posix_input_true = 0;
		while(!posix_input_true){							
			printf("Enter SBG lever arm X\n");
			scanf("%f", &(sbg_config.lever_x));
			if ((sbg_config.lever_x <= 5.0) || (sbg_config.lever_x >= -5.0)) posix_input_true = 1;
			else{
				system_clear_screen();
				printf("Error. Try entering a valid SBG lever arm X.\n");
			}																
		}
		posix_input_true = 0;
		while(!posix_input_true){							
			printf("Enter SBG lever arm Y\n");
			scanf("%f", &(sbg_config.lever_y));
			if ((sbg_config.lever_y <= 5.0) || (sbg_config.lever_y >= -5.0)) posix_input_true = 1;
			else{
				system_clear_screen();
				printf("Error. Try entering a valid SBG lever arm Y.\n");
			}																
		}
		posix_input_true = 0;
		while(!posix_input_true){							
			printf("Enter SBG lever arm z\n");
			scanf("%f", &(sbg_config.lever_z));
			if ((sbg_config.lever_z <= 5.0) || (sbg_config.lever_z >= -5.0)) posix_input_true = 1;
			else{
				system_clear_screen();
				printf("Error. Try entering a valid SBG lever arm z.\n");
			}																
		}