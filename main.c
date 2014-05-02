#include <stdio.h>
#include <htc.h>
#include "lcd.h"
#include <string.h>
#include <stdlib.h>
#include "sci.h"
#include <math.h>
#define _XTAL_FREQ 20000000

__CONFIG( HS & WDTDIS & PWRTEN & BOREN & LVPDIS & WRTEN & DEBUGDIS & UNPROTECT );


//Global variables... shouldn't really have these but it works!
unsigned char rxstring[80];
unsigned char * pos;	//Pointer for string position
unsigned char FEC[3]="12";
unsigned char SR[5]="4000";
unsigned char VPID[5]="0256";
unsigned char APID[5]="0257";
unsigned char PCRPID[5]="0256";
unsigned char PMTPID[5]="4095";	//not used yet
unsigned char CALLSIGN[11]="M0DTS     ";
unsigned char FREQ[16]="12800";  //16bit int to string with itoa
unsigned char MODE=0;
unsigned char INPUT=1;
unsigned char PGMTITLE[16]="TITLE          ";
unsigned char PGMTEXT[16]="TEXT           ";




void delayS(unsigned char seconds){		// Delay function to get roughly seconds
	for (unsigned char a=0;a<seconds;a++){
		for (unsigned char n=0;n<100;n++){
		__delay_ms(10);
		}
	}
}

void delaymS(unsigned char msec){		// Delay function to get roughly milliseconds
		for (unsigned char n=0;n<msec;n++){
		__delay_ms(1);
		}
}




//SPI routine for Ultram VCO
void spi_write(long int reg){
	//Data = RA0
	//Clock = RA1
	//Load = RA2


	long int tmp;		//24bits of 32bit int used for VCO registers
	unsigned char n;
	for(n=24;n>0;n--){
		tmp = 1;
		tmp = tmp << (n-1);		//create mast for bit selection
		tmp = reg & tmp;		//AND mask with register
		tmp = tmp >> n-1;  		//Shift selected bit to LSB
		RA0 = tmp;				//output bit to DATA port
		delaymS(1);

		//Clock
		RA1=1;
		delaymS(1);
		RA1=0;
	}

	//Load 
	RA2=1;
	delaymS(1);
	RA2=0;
}




void putfreq(void){
		lcd_putch(FREQ[0]);
		lcd_putch(FREQ[1]);
		lcd_putch(FREQ[2]);
		lcd_putch(FREQ[3]);
		lcd_putch('.');
		lcd_putch(FREQ[4]);
}

void putcall(void){
	for(unsigned char n=0;n<10;n++){lcd_putch(CALLSIGN[n]);}
}



//Source display modes function to save space!
void srcopts(void){		
	switch(MODE){
		case 0:
			lcd_puts("Live");
			break;
		case 1:
			lcd_puts("SDcard");
			break;
		case 2:
			lcd_puts("Carrier   ");
			break;
		case 3:
			lcd_puts("LSB       ");
			break;
		case 4:
			lcd_puts("USB       ");
			break;
		case 5:
			lcd_puts("In Phase  ");
			break;
		case 6:
			lcd_puts("ALT1      ");
			break;
		case 7:
			lcd_puts("ALT2      ");
			break;
		case 8:
			lcd_puts("ALT4      ");
			break;
	}


}



// Request settings from Digilite
unsigned char getDLsettings(void){
	unsigned char FECcheck[3];
	unsigned char SRcheck[5];
	sci_PutByte('A');
	sci_PutByte(':');
	sci_PutByte('?');
	sci_PutByte(13);

	unsigned char n = 0;
	while(1){
		if(FERR){					//if frame error get errored byte to clear error
			sci_GetByte();
		}else{
			rxstring[n] = sci_GetByte();		//grab next byte
			if (rxstring[n] == ']'){break;}		//jump out of while if end if line unsigned char received (']')
			if (rxstring[n] == 1){break;}		//break on timeout return from get byte  //buggy!
			if(n>78){break;}					// to stop over-flow of buffer!
			sci_CheckOERR();					//check for overrun error, if exists will clear.
			n++;		
		}
	}
//	pos = strstr(rxstring,"[A:100 Version");		//check received string is valid
	if(strstr(rxstring,"[A:100 Version")){
		pos = strstr(rxstring,"SR = ");	//Wait for string within rx data	
		for (n=0;n<4;n++){	//extract required unsigned chars from string
			SRcheck[n]=*(pos+n+5);
		}
		pos = strstr(rxstring,"FEC = ");	//Wait for string within rx data		
		FECcheck[0]=*(pos+6);
		FECcheck[1]=*(pos+8);	//skip '/'
	}
	
	int test;
	int test2;
	test = FECcheck[0] + FECcheck[1] + SRcheck[0] + SRcheck[1] + SRcheck[2] + SRcheck[3];
	test2 = FEC[0] + FEC[1] + SR[0] + SR[1] + SR[2] + SR[3];

	if(test == test2){
		return 0;		//found, continue
	}else{
		lcd_gotorow(2);
		lcd_puts("DigiLite ***");
		delaymS(250);
		return 1;		//not found, retry
	}
		
}
	


//changes digilite settings
void setDL(void){


	sci_PutByte('A');	//send update request string to DigiLite containing new SR and FEC
	sci_PutByte(':');
	sci_PutByte('S');
	sci_PutByte('R');
	sci_PutByte('F');
	for(unsigned char n=0;n<4;n++){
		sci_PutByte(SR[n]);
	}
	sci_PutByte(',');
	if(MODE>1){			//IF test modes selected
		sci_PutByte('-');
		sci_PutByte(MODE-1+48);
		sci_PutByte(13);	
	}else{
		sci_PutByte(FEC[0]);
		sci_PutByte(13);
		delaymS(250);

		while(getDLsettings()){	//check new settings have been accepted, repeats as sometimes timing issue*****
			setDL();
		}	
	}

}



void get808string(void){
	unsigned char n = 0;

	while(1){
		rxstring[n] = getch();	//grab next byte from SOFTWARE serial port
		if (rxstring[n] == '.'){break;}	//jump out of while if  end if line unsigned char received ('.')
		if (rxstring[n] == 1){break;}
		if(n>78){break;}
		n++;
	}
}




//send SR,FEC,Video PID,Audio PID,PVR Input,Callsign,Program Title,Program Text to MK808
void send808string(void){		

	if (MODE ==0){
		unsigned char n;
		putch('#');							
		putch('#');
		for(n=0;n<4;n++){putch(SR[n]);}
		putch(',');
		putch(FEC[0]);
		putch('/');
		putch(FEC[1]);
		putch(',');
		for(n=0;n<4;n++){putch(VPID[n]);}
		putch(',');	
		for( n=0;n<4;n++){putch(APID[n]);}
		putch(',');
		for( n=0;n<4;n++){putch(PMTPID[n]);}		//not used at present
		putch(',');
		putch(INPUT+48);
		putch(',');
		for(n=0;n<10;n++){putch(CALLSIGN[n]);}
		putch(',');
		for(n=0;n<15;n++){putch(PGMTITLE[n]);}
		putch(',');
		for(n=0;n<15;n++){putch(PGMTEXT[n]);}
		putch(10);	//newline unsigned char
	}else{
		putch('s');
		putch('t');
		putch('o');
		putch('p');
		putch(10);	//newline unsigned char
	}

}


//Set LO Frequency	
void setLO (unsigned char * FREQ){

	
	int F;
	F = atoi(FREQ);		//convert FREQ string to int to allow calculation of registers
	
	long int B;  		//long needed for left shift below
	unsigned char A;
	B = F/32;			//B from frequecny 
	A = F - (32*B);		//A from frequency and B

	long int N;
	N = A<<2 | (B<<8) | 2;  //Creates N register from A and B.

	spi_write(3146129); 	//R reg  1100000000000110010001  //100KHz step
	delaymS(5);
	spi_write(9431332); 	//Control reg - see BATC forum for details or check out the docs! ;-)
	delaymS(5);
	spi_write(N); 			//N reg    

}



//Main routine for updating LCD
void MainDisplay(void){

	lcd_clear();
	lcd_gotorow(1);	// select first line
	putcall();

	switch(MODE){
		case 0:
			lcd_puts("Live");
			break;
		case 1:
			lcd_puts("SDcard");
			break;
		default:
			lcd_puts("TstMod");
			break;
	}





	lcd_gotorow(2);	// select first line

	if(MODE==0){
		lcd_puts(SR);
		lcd_putch(' ');
		lcd_putch(FEC[0]);
		lcd_putch('/');
		lcd_putch(FEC[1]);
		lcd_putch(' ');
		lcd_putch(' ');
		putfreq();	//function to save space!
	}
	if(MODE==1){
		lcd_puts("SD SR/FEC ");
		putfreq();
	}
	
	if(MODE>1){
		srcopts();	//function to save space!
		putfreq();	//function to save space!
	}
}



//Config Digilite, MK808 and LO
void configure(void){
	lcd_clear();
	lcd_gotorow(1);
	lcd_puts("Configuring");
	lcd_gotorow(2);
	lcd_puts("DigiLite");


	putch('s');	//Stop encoder to allow comms with DigiLite PIC. needed otherwise DigiLite comms unreliable.
	putch('t');
	putch('o');
	putch('p');
	putch(10);



//Set SDcard source if required		//Stop SD mode before sending any settings!!
	if(MODE==1){
		RA3 = 0;
	}else{
		RA3=1;
	}

	delayS(1);



//Setup DigiLite if in Live or Test modes, Do nothing if in SD card mode.
	if(MODE==0){
	setDL();
	}

	if(MODE>1){
	setDL();
	}



	lcd_clear();
	lcd_gotorow(1);
	lcd_puts("Configuring");
	lcd_gotorow(2);
	lcd_puts("MK808");

	delayS(1);


//send MK808 settings... 
	unsigned char n = 0;
	while(!n){			
		send808string();
		get808string();	//get string from MK808
		if(MODE==0){
			pos = strstr(rxstring,"Config ok");	//check it is valid
			if(pos){n=1;}

		}else{
			pos = strstr(rxstring,"Stopped");	//check it is valid
			if(pos){n=1;}
		}
	}

//Set LO frequency
	setLO(FREQ);

//Main display output
	MainDisplay();


}




//Menu actions.........
void menu (void){

	int SRopt[8] = {1000,1333,1500,1666,2000,4000,4166};		//Digilite cannot do exactly 4167 hence 4166(.667) but it works fine, 1667 is the same.
	int FECopt[6] = {12,23,34,56,78};



	unsigned char cursorpos;
	unsigned char changed;
	unsigned char n;
	changed = 0;



//SR menu option
	if(RC0==0){			//Menu button pressed
		lcd_clear();
		lcd_gotorow(1);	// select first line
		lcd_puts("Symbol Rate");
		lcd_gotorow(2);	// select first line
		lcd_puts(SR);

		
		


		while(RC2){		//while Enter NOT pressed
			if(RC1==0){	//when adjust pressed
				delaymS(200);
				unsigned char tmp[16];
				n=0;
				while(n<7){
					itoa(tmp,SRopt[n],10);
					if(strcmp(tmp,SR)==0){
						if(n==6){
							itoa(tmp,SRopt[0],10);
							SR[0] = tmp[0];
							SR[1] = tmp[1];
							SR[2] = tmp[2];
							SR[3] = tmp[3];
							break;
						}else{
							itoa(tmp,SRopt[(n+1)],10);
							SR[0] = tmp[0];
							SR[1] = tmp[1];
							SR[2] = tmp[2];
							SR[3] = tmp[3];
							break;
						}
					}
					n++;
				}

				lcd_gotorow(1);	// select first line
				lcd_puts("Symbol Rate");
				lcd_gotorow(2);	// select first line
				lcd_puts(SR);
				changed=1;
			}	
		}


	


//FEC menu option
		lcd_clear();
		lcd_gotorow(1);	// select first line
		lcd_puts("FEC");
		lcd_gotorow(2);	// select first line

		
		lcd_putch(FEC[0]);
		lcd_putch('/');
		lcd_putch(FEC[1]);
		
		delaymS(200);



		while(RC2){		//while Enter NOT pressed
			if(RC1==0){	//when adjust pressed
				delaymS(200);
				unsigned char tmp[16];
				n=0;

				while(n<5){
					itoa(tmp,FECopt[n],10);

					if(strcmp(tmp,FEC)==0){

						if(n==4){
							itoa(tmp,FECopt[0],10);
							FEC[0] = tmp[0];
							FEC[1] = tmp[1];
							break;
						}else{
							itoa(tmp,FECopt[n+1],10);
							FEC[0] = tmp[0];
							FEC[1] = tmp[1];
							break;
						}
					}
					n++;
				}

				lcd_gotorow(1);	// select first line
				lcd_puts("Set FEC");
				lcd_gotorow(2);	// select first line

				lcd_putch(FEC[0]);
				lcd_putch('/');
				lcd_putch(FEC[1]);
				
				changed=1;
			}
				
		}




//CALLSIGN menu option
		lcd_clear();
		lcd_gotorow(1);	// select first line
		lcd_puts("Callsign");
		lcd_gotorow(2);	// select second line	
		putcall();

		lcd_putch('<');
		delaymS(200);
		lcd_gotorow(2);
		


		cursorpos = 0;
		while(RC2){		//while Enter NOT pressed
			if(RC1==0){	//when adjust pressed
				delaymS(200);
				lcd_write(0x0E);	
				while(cursorpos < 10){
					delaymS(250);
					while(RC2){		//while Enter NOT pressed
						if(RC1==0){	//when adjust pressed
							delaymS(200);
							if(CALLSIGN[cursorpos] > 89){
									CALLSIGN[cursorpos] = 32;
							}else{
								if(CALLSIGN[cursorpos] == 32){
									CALLSIGN[cursorpos] = 47;
								}else{
									CALLSIGN[cursorpos] ++;
								}
							}
							lcd_gotorow(2);	// select second line	
							for(n=0;n<10;n++){
								lcd_putch(CALLSIGN[n]);
							}
							lcd_putch('<');
							lcd_goto(0x80 + 0x40 + cursorpos);
							changed=1;
						}
					}
					cursorpos++;
					lcd_goto(0x80 + 0x40 + cursorpos);
				}
				lcd_write(0x0C);  //turn off cursor
			}
		}





//Event Title menu option
		lcd_clear();
		lcd_gotorow(1);	// select first line
		lcd_puts("Program Title");
		lcd_gotorow(2);	// select second line	
		lcd_puts(PGMTITLE);

		lcd_putch('<');
		delaymS(200);
		lcd_gotorow(2);
		


		cursorpos = 0;
		while(RC2){		//while Enter NOT pressed
			if(RC1==0){	//when adjust pressed
				delaymS(200);
				lcd_write(0x0E);	
				while(cursorpos < 15){
					delaymS(250);
					while(RC2){		//while Enter NOT pressed
						if(RC1==0){	//when adjust pressed
							delaymS(200);
							if(PGMTITLE[cursorpos] > 89){
									PGMTITLE[cursorpos] = 32;
							}else{
								if(PGMTITLE[cursorpos] == 32){
									PGMTITLE[cursorpos] = 47;
								}else{
									PGMTITLE[cursorpos] ++;
								}
							}
							lcd_gotorow(2);	// select second line	
							lcd_puts(PGMTITLE);
							lcd_putch('<');
							lcd_goto(0x80 + 0x40 + cursorpos);
							changed=1;
						}
					}
					cursorpos++;
					lcd_goto(0x80 + 0x40 + cursorpos);
				}
				lcd_write(0x0C);  //turn off cursor
			}
		}



//Event Text menu option
		lcd_clear();
		lcd_gotorow(1);	// select first line
		lcd_puts("Program Text");
		lcd_gotorow(2);	// select second line	
		lcd_puts(PGMTEXT);


		lcd_putch('<');
		delaymS(200);
		lcd_gotorow(2);
		


		cursorpos = 0;
		while(RC2){		//while Enter NOT pressed
			if(RC1==0){	//when adjust pressed
				delaymS(200);
				lcd_write(0x0E);	
				while(cursorpos < 15){
					delaymS(250);
					while(RC2){		//while Enter NOT pressed
						if(RC1==0){	//when adjust pressed
							delaymS(200);
							if(PGMTEXT[cursorpos] > 89){
									PGMTEXT[cursorpos] = 32;
							}else{
								if(PGMTEXT[cursorpos] == 32){
									PGMTEXT[cursorpos] = 47;
								}else{
									PGMTEXT[cursorpos] ++;
								}
							}
							lcd_gotorow(2);	// select second line	
							lcd_puts(PGMTEXT);
							lcd_putch('<');
							lcd_goto(0x80 + 0x40 + cursorpos);
							changed=1;
						}
					}
					cursorpos++;
					lcd_goto(0x80 + 0x40 + cursorpos);
				}
				lcd_write(0x0C);  //turn off cursor
			}
		}






//Video Pid menu option
		lcd_clear();
		lcd_gotorow(1);	// select first line
		lcd_puts("Video PID");
		lcd_gotorow(2);	// select second line	
		lcd_puts(VPID);
		delaymS(200);
		lcd_gotorow(2);



		cursorpos = 0;
		while(RC2){		//while Enter NOT pressed
			if(RC1==0){	//when adjust pressed
				delaymS(200);
				lcd_write(0x0E);
				while(cursorpos < 4){
				delaymS(250);
					while(RC2){		//while Enter NOT pressed
						if(RC1==0){	//when adjust pressed
							delaymS(200);
							if(VPID[cursorpos] > 56){ //9
									VPID[cursorpos] = 48;  //0
							}else{
								VPID[cursorpos] ++;
							}
							lcd_gotorow(2);	// select second line	
							lcd_puts(VPID);
							lcd_goto(0x80 + 0x40 + cursorpos);
							changed=1;
						}
					}
					cursorpos++;
					lcd_goto(0x80 + 0x40 + cursorpos);
				}
				lcd_write(0x0C);  //turn off cursor
			}
		}


//Audio Pid menu option
		lcd_clear();
		lcd_gotorow(1);	// select first line
		lcd_puts("Audio PID");
		lcd_gotorow(2);	// select second line	
		lcd_puts(APID);
		delaymS(200);
		lcd_gotorow(2);



		cursorpos = 0;
		while(RC2){		//while Enter NOT pressed
			if(RC1==0){	//when adjust pressed
				delaymS(200);
				lcd_write(0x0E);
				while(cursorpos < 4){
				delaymS(250);
					while(RC2){		//while Enter NOT pressed
						if(RC1==0){	//when adjust pressed
							delaymS(200);
							if(APID[cursorpos] > 56){ //9
									APID[cursorpos] = 48;  //0
							}else{
								APID[cursorpos] ++;
							}
							lcd_gotorow(2);	// select second line	
							lcd_puts(APID);
							lcd_goto(0x80 + 0x40 + cursorpos);
							changed=1;
						}
					}
					cursorpos++;
					lcd_goto(0x80 + 0x40 + cursorpos);
				}
				lcd_write(0x0C);  //turn off cursor
			}
		}




	




//Source menu option
		lcd_clear();
		lcd_gotorow(1);	// select first line
		lcd_puts("Tx Modes");
		lcd_gotorow(2);	// select second line	
		srcopts();
		delaymS(250);




		while(RC2){		//while Enter NOT pressed
			if(RC1==0){	//when adjust pressed
				delaymS(200);
				if(MODE==8){
					MODE=0;
				}else{
					MODE++;
				}
				lcd_clear();
				lcd_gotorow(1);	// select first line
				lcd_puts("Tx Modes");
				lcd_gotorow(2);	// select second line	
				srcopts();
				
				changed = 1;
			}
		}




//Input menu option
		lcd_clear();
		lcd_gotorow(1);	// select first line
		lcd_puts("PVR Video Input");
		lcd_gotorow(2);	// select second line	
		if(INPUT==1){
			lcd_puts("Composite");
		}else{
			lcd_puts("S-Video");
		}
		delaymS(250);




		while(RC2){		//while Enter NOT pressed
			if(RC1==0){	//when adjust pressed
				delaymS(200);
				if(INPUT==1){
					INPUT=2;
				}else{
					INPUT=1;
				}
				lcd_clear();
				lcd_gotorow(1);	// select first line
				lcd_puts("PVR Video Input");
				lcd_gotorow(2);	// select second line	
				if(INPUT==1){
					lcd_puts("Composite");
				}else{
					lcd_puts("S-Video");
				}
				changed = 1;
			}
		}


//Frequency menu option
		lcd_clear();
		lcd_gotorow(1);	// select first line
		lcd_puts("Frequency");
		lcd_gotorow(2);	// select second line	
		putfreq();
		lcd_puts("MHz");
		delaymS(200);
		lcd_gotorow(2);
		


		cursorpos = 1;
		lcd_goto(0x80 + 0x40 + cursorpos);


		while(RC2){		//while Enter NOT pressed
			if(RC1==0){	//when adjust pressed
				delaymS(200);
				lcd_write(0x0E);
				while(cursorpos < 6){
				delaymS(250);
					while(RC2){		//while Enter NOT pressed
						if(RC1==0){	//when adjust pressed
							delaymS(200);
							if(cursorpos < 5){
								if(FREQ[cursorpos] > 56){	//9
										FREQ[cursorpos] = 48; 	//0
								}else{
									FREQ[cursorpos] ++;
								}
							}else{
								if(FREQ[cursorpos-1] > 56){
									FREQ[cursorpos-1] = 48; 
								}else{
									FREQ[cursorpos-1] ++;
								}
		
							}
							lcd_gotorow(2);	// select second line	
							putfreq();
								lcd_goto(0x80 + 0x40 + cursorpos);
							changed=1;
						}
					}
					cursorpos++;
					if (cursorpos == 4){ cursorpos++;}	//skip '.'
					lcd_goto(0x80 + 0x40 + cursorpos);
				}
				lcd_write(0x0C);  //turn off cursor
			}
		}


//Save settings
		if (changed ==1){
			eeprom_write(0,FEC[0]);
			eeprom_write(1,FEC[1]);
			for(n=0;n<4;n++){eeprom_write(n+2,SR[n]);}
			for(n=0;n<4;n++){eeprom_write(n+6,VPID[n]);}
			for(n=0;n<4;n++){eeprom_write(n+10,APID[n]);}
			for(n=0;n<10;n++){eeprom_write(n+14,CALLSIGN[n]);}
			for(n=0;n<5;n++){eeprom_write(n+24,FREQ[n]);}
			eeprom_write(29,MODE);	
			eeprom_write(30,INPUT);
			for(n=0;n<15;n++){eeprom_write(n+31,PGMTITLE[n]);}
			for(n=0;n<15;n++){eeprom_write(n+46,PGMTEXT[n]);}
			

			lcd_clear();
			lcd_gotorow(1);	// select first line
			lcd_puts("Saved Changes");
			delayS(1);
			
			//Update DigiLite and MK808 settings
			configure();
	
		}else{
			lcd_clear();
			lcd_gotorow(1);	// select first line
			lcd_puts("No Changes");
			delayS(1);
			MainDisplay();
		}
	}
}



void main(void){


//PIC initialisation of ports ADC,Comparator etc...
	TRISB=0;
	TRISC7=1;
	TRISC6=0;
	TRISC5=1;
	TRISC4=0;
	TRISA0=0;
	TRISA1=0;
	TRISA2=0;
	TRISA3=0;	
	ADCON1 = 6;	// Disable analog pins on PORTA
	CMCON=7;
	RA0=0;	//VCO Data
	RA1=0;	//VCO Clock
	RA2=0;	//VCO Load
	RA3=1;	//SDCard select *Inverted

//set tinterrupts for timer0  used for software serial port
	OPTION = 0b0111;	// prescale by 256
	T0CS = 0;			// select internal clock
	T0IE = 1;			// enable timer interrupt


//Initialise comm and lcd
	sci_Init(57600,0);	//hardware UART serial port to talk to DigiLite
	lcd_init();		// intialise LCD




 //////////////////////////////////////////////////////////////////////
//init defualt settings
if(eeprom_read(0)<255){ 	//create default settings if eeporm is empty
	FEC[0] = eeprom_read(0);
	FEC[1] =  eeprom_read(1);
	for(unsigned char n=0;n<4;n++){SR[n]= eeprom_read(n+2);}
	for(unsigned char n=0;n<4;n++){VPID[n]= eeprom_read(n+6);}
	for(unsigned char n=0;n<4;n++){APID[n]= eeprom_read(n+10);}
	for(unsigned char n=0;n<10;n++){CALLSIGN[n]= eeprom_read(n+14);}
	for(unsigned char n=0;n<5;n++){FREQ[n]= eeprom_read(n+24);}
	MODE = eeprom_read(29);
	INPUT = eeprom_read(30);
	for(unsigned char n=0;n<15;n++){PGMTITLE[n]= eeprom_read(n+31);}
	for(unsigned char n=0;n<15;n++){PGMTEXT[n]= eeprom_read(n+46);}
}





//Display boot up text
	lcd_gotorow(1);	// select first line
	delayS(1);
	lcd_puts("DigiLite + MK808");
	lcd_gotorow(2);	// Select second line
	lcd_puts("Controller.");
	delayS(3);

//wait for MK808 to boot up...
	lcd_clear();
	lcd_gotorow(1);	// select first line
	lcd_puts("Please Wait");
	lcd_gotorow(2);	// select first line
	lcd_puts("MK808 start up..");
	delayS(1);


//	lcd_clear();
//	lcd_gotorow(1);	// select first line
//	lcd_puts("Please Wait");
//	lcd_gotorow(2);	// select first line

//	for(unsigned char n=0;n<16;n++){		//pointless loading indicater...!
//		lcd_puts(".");
//		delayS(1);
//	}	
	

	unsigned char n=0;
	while (!n){		//Wait for response from MK808 after sending 'start' command...
		putch('s');
		putch('t');
		putch('a');
		putch('r');
		putch('t');
		putch(13);
		putch(10);
		get808string();						//get string from MK808 when it's ready...
		pos = strstr(rxstring,"started");	//check it is valid
		if(pos){n=1;}
	}

	//setup DigiLite and MK808
	configure();

	for(;;){
		menu();

	}
	
}




