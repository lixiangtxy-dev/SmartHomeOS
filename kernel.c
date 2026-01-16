#define UARTO_DR ((volatile unsigned char *)0x10000000)

void uart_putc(char c){
    *UARTO_DR=c;
}

void uart_puts(char *s){
    while(*s){
        uart_putc(*s++);
    }
}

void main(){
    uart_puts("Hello SmartHomeOS！\n");
    uart_puts("System Booted Successfully\n");
    while(1){}
}