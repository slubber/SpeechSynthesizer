#include <stdio.h>
#include <string.h>

char found[16];

int vertaile (char ip[])
{

int i=0;
int lippu=0;

char unknownmsg[14] = {"##UNKNOWN##"};



char *ipnick[9][2] = { 

						{"10.10.10.250","@Tap"}, 
						{"10.10.10.241","@Apamobil"},
						{"10.10.10.242","@Heavy"},
						{"10.10.10.243","@Milk"},
						{"10.10.10.244","@Kuola"},
						{"10.10.10.251","@Pablo^2"},
						{"10.10.10.252","@Douche_bag"},
						{"10.10.10.253","@Lesta"},
						
						};
						

printf("...searching for %s\n", ip); 
for (i=0; i<8; i++)
	{
		if (strcmp(ipnick[i][0], ip)==0) {
		strcpy (found, ipnick[i][1]);
		lippu = 1;
		}
	}
	
if (lippu ==0)
{
strcpy (found, unknownmsg);
return 2;
}
else	
return 1;

}


int main (){


char ip[16];
int nick;
printf("Ip:");
gets(ip);
nick=vertaile(ip); 	/*Ip aliohjelmalle*/
printf("\nFound: %s <--> %s \n\n",ip,found);

return 0;

}

