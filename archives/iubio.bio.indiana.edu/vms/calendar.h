struct Calstruct {
     char Time[15];
     char Location[MAXLINE];
     char Summary[MAXLINE]; 
     char Recurs;
     char expires[15];
     int lengthmin;
     int lengthmax;
     char Exceptions[MAXLINE];
     char Notify[MAXLINE];
     char Status;
     
     };

typedef struct Calstruct Calendar;
