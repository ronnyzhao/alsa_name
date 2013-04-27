#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <vector>
#include <string>

using namespace std;
 
//Simple class/structure that will hold the pairs of USB ports and
//ALSA card numbers.
class CardPortPair
{
public:
	int cardNum;
	string portName;
};

//Function declarations
int GetDValue(const char *k) ;
bool isnum(char c);
int GetCardList(vector<CardPortPair> &list);
int UdevGetCardNumberForPort(char *k, char *n);
int CreateAlsaMultiCardConfig(void);

int main(int argc, char **argv)
{
       if (argc == 1) //no parameters passed
		return CreateAlsaMultiCardConfig();
	if (argc != 3)  //wrong number of parameters
		return -1;
	
	///////////////////////////////////////////////////////////////////////////////////////////////////////
	//
	// Create a file in /etc/udev/rules.d (this is the folder in Gentoo) that comes before
	// the Alsa configuration in alphabetical order.  For example, in gentoo I made this file:
	//
	// File: /etc/udev/rules.d/39-usb-alsa.rules
	//
	// KERNEL=="controlC[0-9]*", DRIVERS=="usb", PROGRAM="/usr/bin/alsa_name %k %b", NAME="snd/%c{1}"
	// KERNEL=="hwC[D0-9]*", DRIVERS=="usb", PROGRAM="/usr/bin/alsa_name %k %b", NAME="snd/%c{1}"
	// KERNEL=="midiC[D0-9]*", DRIVERS=="usb", PROGRAM="/usr/bin/alsa_name %k %b", NAME="snd/%c{1}"
	// KERNEL=="pcmC[D0-9cp]*", DRIVERS=="usb", PROGRAM="/usr/bin/alsa_name %k %b", NAME="snd/%c{1}"
	//
	// What this means is that when udev detects a new USB device that matches the KERNEL name (i.e.
	// "controlC2" or "pcmC0D1", it will execute this program, which must be saved and executable
	// as/usr/bin/alsa_name.  Udev will pass the KERNEL name as well as the usb port name as the
	// first two parameters to this program
	//
	///////////////////////////////////////////////////////////////////////////////////////////////////////
	
	///////////////////////////////////////////////////////////////////////////////////////////////////////
	//
	// This program uses a simple configuration file to load the BUS address/card number pairs
	// so that whenever a USB sound card device gets plugged into a certain USB port, it will
	// always be assigned the same card number.  Do not use a card number <0 or >31
	//
	// File: /etc/audiomux_ports
	//
	// "3-2" 4    #This means that USB port "3-2" will get assigned alsa card number 4
	// "1-1.2" 3  #And USB port "1-1.2" will get assigned alsa card number 3
	//
	////////////////////////////////////////////////////////////////////////////////////////////////////
	
	

	//Arg1 is the kernel device name (i.e. "controlC0" or "pcmC0D0p")
	//Arg2 is the KERNELS name, for usb this is the port name (i.e. "3-2" or "1-1.2")
	return UdevGetCardNumberForPort(argv[1], argv[2]);	
}

//Search for matching USB port/Alsa cardnum pairs and print the appropriate
//value to stdio so that udev can retrieve the value and set the appropriate
//entry.
int UdevGetCardNumberForPort(char *k, char *n)
{
	vector<CardPortPair> list;
	GetCardList(list);
	string sn = n;

	int size = list.size();
	for (int i = 0; i < size; ++i) {
		if (list[i].portName == sn) {
			if (!strncmp("controlC", k, 8)) {  //control interface
				printf("controlC%d\n", list[i].cardNum);
				return 0;
			} else if (!strncmp("pcmC", k, 4)) { //playback or capture interface				
				if (k[strlen(k) - 1] == 'p') {
					printf("pcmC%dD%dp\n", list[i].cardNum, GetDValue(k));  //Recreate the name w/ our custom card number
					return 0;
				} else if (k[strlen(k) - 1] == 'c') {
					printf("pcmC%dD%dc\n", list[i].cardNum, GetDValue(k));  //Recreate the name w/ our custom card number
					return 0;
				}
				return -1;
			} else if (!strncmp("hwC", k, 3)) {  //THIS IS UNTESTED SINCE I DON'T HAVE A USB CARD WITH hwC INTERFACE				
				printf("hwC%dD%d\n", list[i].cardNum, GetDValue(k));				
			} else if (!strncmp("midiC", k, 5)) { //THIS IS UNTESTED SINCE I DON'T HAVE A USB CARD WITH midiC INTERFACE				
				printf("midiC%dD%d\n", list[i].cardNum, GetDValue(k));
			}
		}
	}

	return -1;
}

//Retrieve the subdevice value, whith comes after the capital D
//in the device name, (i.e. pcmC0D1).
int GetDValue(const char *k) 
{
	const char *p = strrchr((const char *)k, 'D');
	if (!p) {
		return -1;
	}
	p++;
	int d = strtol(p, NULL, 0);
	if (d < 0 || d > 32)
		return 0;
	
	return d;
}

//Retrieve a list of USB port name/Alsa card number pairs
//from a file located at "/etc/audiomux_ports".
//If a USB sound card is detected at a certain USB port, it will
//always be assigned the matching card number
int GetCardList(vector<CardPortPair> &list)
{
	char buffer[500];
	FILE *f = fopen("/etc/audiomux_ports", "r");
	if (!f)
		return -1;

	while (!feof(f)) {
		char *p = fgets(buffer, 500, f);
		if (!p)
		   continue;

		//Skip to first quote
		while (*p && *p != '\"') {
			p++;	
		}
		if (!*p) {
			printf("Invalid line received (1)\n");
			continue;
		}
		p++;
		
		//Skip to 2nd quote
		char *p2 = p;
		while (*p2 && *p2 != '\"') {
			p2++;
		}
		if (!*p2) {
			printf("Invalid line received (2)\n");
			continue;
		}
		*p2 = 0;

		//Retrieve name
		string name = p;

		//Skip to channel number
		p = p2 + 1;
		while (*p && !isnum(*p)) {
			p++;
		}
		if (!*p) {
			printf("Invalid line received (3)\n");
			continue;
		}
		int num = strtol(p, NULL, 0);
		if (num < 1 || num > 31) {
			printf("Invalid cardNum received\n");
			continue;
		}

		CardPortPair c;
		c.cardNum = num;
		c.portName = name;
		list.push_back(c);
	}

	fclose(f);

}

//I couldn't remember the C function to detect numeric chars!!
bool isnum(char c) {
	if (c < '0' || c > '9') return false;
	return true;
}

//This function creates the /etc/asound.conf file that is necessary
//to combine all USB sound cards defined by the /etc/audiomux_ports
//file into a single combined "sound-card".  This assumes that the
//sound cards each only have one stereo playback device each.
//
//This is necessary to use all the sound cards in JACK.  In JACK,
//the alsa sound device to use will be "ttable".
int CreateAlsaMultiCardConfig()
{
	FILE *f = fopen("/etc/asound.conf", "w");
	if (!f) {
		printf("Unexpected error! Could not open .asoundrc");
		return -1;
	}

	vector<CardPortPair> list;
	GetCardList(list);

	fprintf(f, "pcm.multi {\n");
	fprintf(f, "\ttype multi;\n");

	int size = list.size();
	for (int i = 0; i < size; ++i) {
		fprintf(f, "\tslaves.%c.pcm \"hw:%d,0\";\n", 'a' + i, list[i].cardNum);
		fprintf(f, "\tslaves.%c.channels 2;\n", 'a' + i);
	}

	for (int i = 0; i < size; ++i) {		
		fprintf(f, "\tbindings.%d.slave %c;\n", i * 2, 'a' + i);
		fprintf(f, "\tbindings.%d.channel 0;\n", i * 2);
		fprintf(f, "\tbindings.%d.slave %c;\n", i * 2 + 1, 'a' + i);
		fprintf(f, "\tbindings.%d.channel 1;\n", i * 2 + 1);		
	}

	fprintf(f, "}\n");
	fprintf(f, "\n");
	fprintf(f, "ctl.multi {\n");
	fprintf(f, "\ttype hw;\n");
	fprintf(f, "\tcard 0;\n");
	fprintf(f, "}\n");
	fprintf(f, "\n");
	fprintf(f, "pcm.ttable {\n");
	fprintf(f, "\ttype route;\n");
	fprintf(f, "\tslave.pcm \"multi\";\n");
	
	for (int i = 0; i < size; ++i) {
		fprintf(f, "\tttable.%d.%d 1;\n", i * 2, i * 2);
		fprintf(f, "\tttable.%d.%d 1;\n", i * 2 + 1, i * 2 + 1);
	}

	fprintf(f, "}\n");
	fprintf(f, "\n");
	fprintf(f, "ctl.ttable {\n");
	fprintf(f, "\ttype hw;\n");
	fprintf(f, "\tcard 0;\n");
	fprintf(f, "}\n");


 
	fclose(f);

	return 0;
}

