#include <stdio.h>
#include <limits.h>

int main(void) 
{
    /* input variable */
    char line[101];
    long long num = 0;
    
    /* get input from user */
    printf("ml' nob:\n");
    
    /* check if we can read input */
    if(fgets(line, 101, stdin) == NULL) {
        printf("Neh mi'\n");
        return 1;
    }
    
    /* check for invalid characters before trying to read number */
    int i = 0;
    
    /* skip leading spaces */
    while(line[i] == ' ' || line[i] == '\t') {
        i++;
    }
    
    /* check if empty or just spaces */
    if(line[i] == '\n' || line[i] == '\0') {
        printf("Neh mi'\n");
        return 1;
    }
    
    /* check first character and handle negative numbers */
    int is_negative = 0;
    if(line[i] == '-') {
        is_negative = 1;
        i++;
        /* check if there's a digit after minus */
        if(line[i] < '0' || line[i] > '9') {
            printf("Neh mi'\n");
            return 1;
        }
    } else if(line[i] < '0' || line[i] > '9') {
        printf("Neh mi'\n");
        return 1;
    }
    
    /* validate the entire input before parsing */
    int j = i;
    int has_digit = 0;
    while(line[j] >= '0' && line[j] <= '9') {
        has_digit = 1;
        j++;
    }
    
    if(!has_digit) {
        printf("Neh mi'\n");
        return 1;
    }
    
    /* check for decimal point or invalid characters */
    if(line[j] == '.') {
        printf("bIjatlh 'e' yImev\n");
        return 1;
    }
    
    /* skip trailing spaces */
    while(line[j] == ' ' || line[j] == '\t') {
        j++;
    }
    
    /* check if anything else remains (except newline/null) */
    if(line[j] != '\n' && line[j] != '\0') {
        printf("bIjatlh 'e' yImev\n");
        return 1;
    }
    
    /* parse the number manually */
    while(line[i] >= '0' && line[i] <= '9') {
        long long digit = line[i] - '0';
        
        /* check for overflow */
        if(num > (LLONG_MAX - digit) / 10) {
            printf("bIjatlh 'e' yImev\n");
            return 1;
        }
        
        num = num * 10 + digit;
        i++;
    }
    
    if(is_negative) {
        num = -num;
    }
    
    /* check if number is in valid range (0-8) */
    if(num < 0 || num > 8) {
        printf("Qih mi' %lld\n", num);
        return 1;
    }
    
    /* print first line */
    printf("Qapla'\n");
    
    /* print the quote */
    if(num == 0) {
        printf("noH QapmeH wo' Qaw'lu'chugh yay chavbe'lu' 'ej wo' choqmeH may' DoHlu'chugh lujbe'lu'.\n");
    }
    else if(num == 1) {
        printf("bortaS bIr jablu'DI' reH QaQqu' nay'.\n");
    }
    else if(num == 2) {
        printf("Qu' buSHa'chugh SuvwI', batlhHa' vangchugh, qoj matlhHa'chugh, pagh ghaH SuvwI''e'.\n");
    }
    else if(num == 3) {
        printf("bISeH'eghlaH'be'chugh latlh Dara'laH'be'.\n");
    }
    else if(num == 4) {
        printf("qaStaHvIS wa' ram loS SaD Hugh SIjlaH qetbogh loD.\n");
    }
    else if(num == 5) {
        printf("Suvlu'taHvIS yapbe' HoS neH.\n");
    }
    else if(num == 6) {
        printf("Ha'DIbaH DaSop 'e' DaHechbe'chugh yIHoHQo'.\n");
    }
    else if(num == 7) {
        printf("Heghlu'meH QaQ jajvam.\n");
    }
    else if(num == 8) {
        printf("leghlaHchu'be'chugh mIn lo'laHbe' taj jej.\n");
    }
    
    return 0;
}
