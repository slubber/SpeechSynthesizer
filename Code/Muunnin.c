/*Antin saatanan hieno muuntelija*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {

int i=0;
int j=0;
int k=0;

char lause[200] = {'\0'}; 		/*Sy�tetty lause (Mik� pannaan t�lle pituusrajaksi?)*/
char muunnettu[500] = {'\0'}; 	/*Muunnettu lause*/
char *korvaaja; 				/*T�h�n tallennetaan erikoismerkin korvaava teksti*/

printf ("Anna nyt tulla vaan:");/*N�m� jutut on ihan vaan nyt testauksen vuoksi t�ss�*/
fgets(lause, 200, stdin);	  	/*Luetaan lause k�ytt�j�n sy�tteest�*/
lause[strlen(lause)-1]='\0'; 	/*Eliminoidaan brutaalisti mukaantunkeva rivinvaihto*/

printf ("\nAlkuper�inen lause:%s,pituus:%d merkki�\n",lause,strlen(lause));


for (i=0; i<strlen(lause); i++){
	
	if (lause[i]=='\n'); 		/*En oo ihan varma ett� tarvisko t�t�, mutta eip� tuo n�y haittaavankaan*/
	
	else if ( ((lause[i]>64)&&(lause[i]<91))||((lause[i]>96)&&(lause[i]<123))){
		muunnettu[j]=lause[i]; /*Normikirjaimet ja v�lily�nti*/
		j++;
		}
		
	else if ((lause[i]=='�')||(lause[i]=='�')||(lause[i]=='�')||(lause[i]=='�')||(lause[i]=='�')||(lause[i]=='�')){
		muunnettu[j]=lause[i]; /*Skandit*/
		j++;
		}
	
	else if ((lause[i]==' ')||(lause[i]==',')||(lause[i]=='.')){
		muunnettu[j]=lause[i]; /*Space, pilkku, piste*/
		j++;
		}
	
	/*T�ss� alkaa erikoismerkkien k�sittely*/
	else {
	
		k=0;
	
		if (lause[i]=='�')
			korvaaja="pyk�l�";
	
		else if (lause[i]=='�')
			korvaaja="puoli";
	
		else if (lause[i]=='0')
			korvaaja="nolla";
			
		else if (lause[i]=='1')
			korvaaja="yksi";
		
		else if (lause[i]=='2')
			korvaaja="kaksi";	
			
		else if (lause[i]=='3')
			korvaaja="kolme";
			
		else if (lause[i]=='4')
			korvaaja="nelj�";
			
		else if (lause[i]=='5')
			korvaaja="viisi";
			
		else if (lause[i]=='6')
			korvaaja="kuusi";
			
		else if (lause[i]=='7')
			korvaaja="seitsem�n";	
			
		else if (lause[i]=='8')
			korvaaja="kahdeksan";
			
		else if (lause[i]=='9')
			korvaaja="yhdeks�n";
				
		else if (lause[i]=='+')
			korvaaja="plus";
		
		else if (lause[i]=='-')
			korvaaja="viiva";	
			
		else if (lause[i]=='*')
			korvaaja="t�hti";
		
		else if (lause[i]=='/')
			korvaaja="kauttaviiva";
					
		else if (lause[i]=='@')
			korvaaja="at";
			
		else if (lause[i]=='�')
			korvaaja="puntamerkki";
			
		else if (lause[i]=='�')
			korvaaja="pillumerkki";
			
		else if (lause[i]=='$')
			korvaaja="dollarimerkki";
			
		else if (lause[i]=='%')
			korvaaja="prosenttimerkki";
			
		else if (lause[i]=='&')
			korvaaja="ja";
			
		else if ((lause[i]=='{')||(lause[i]=='}'))
			korvaaja="kaarisulku";
			
		else if ((lause[i]=='(')||(lause[i]==')'))
			korvaaja="sulkumerkki";
			
		else if ((lause[i]=='[')||(lause[i]==']'))
			korvaaja="hakasulku";
			
		else if (lause[i]=='?')
			korvaaja="kysymysmerkki";
			
		else if (lause[i]=='~')
			korvaaja="tilde";
			
		else if (lause[i]=='<')
			korvaaja="on pienempi kuin";
			
		else if (lause[i]=='>')
			korvaaja="on suurempi kuin";
			
		else if (lause[i]=='|')
			korvaaja="putki";
			
		else if (lause[i]=='!')
			korvaaja="huutomerkki";
			
		else if (lause[i]=='#')
			korvaaja="risuaita";
			
		else if (lause[i]=='"')
			korvaaja="lainausmerkki";
			
		else if (lause[i]=='=')
			korvaaja="on yht� kuin";
			
		else if ((lause[i]=='\'')||(lause[i]=='`')||(lause[i]=='�'))
			korvaaja="heittomerkki";
			
		else if (lause[i]=='^')
			korvaaja="hatten";
				
		else if (lause[i]=='�')
			korvaaja="umlaut";
				
		else if (lause[i]=='\\')
			korvaaja="kenoviiva";
			
		else if (lause[i]=='_')
			korvaaja="alaviiva";
			
		else korvaaja="tuntematon merkki";
		
		while (korvaaja[k] != '\0') {
			muunnettu[j] = korvaaja[k];
			k++;
			j++;
			}
				
			
		} 		/*Erikoismerkkien k�sittely*/
	}			/*For*/

printf("\nMuunnettu lause:%s, pituus:%d\n",muunnettu,strlen(muunnettu)); //Tulostetaan testauksen vuoksi muutettu lause

return 0;

}				/*main*/
