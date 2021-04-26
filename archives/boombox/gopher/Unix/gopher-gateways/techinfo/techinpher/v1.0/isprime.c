#include <stdio.h>

main()
{
  char input[80];
  int num, half;
  int factor;
  int prime;

  while (fgets(input,sizeof(input)-1, stdin) != NULL) {
    num = atoi (input);
    do {
      half = num/2;
      for (prime=1, factor=2; factor < half && prime; factor++)
	if ( num % factor == 0 ) prime = 0;
      if (prime) {
	printf ("%d is a prime number\n", num);
      }
      else {
	num++;}
    } while (!prime);
  }
}

