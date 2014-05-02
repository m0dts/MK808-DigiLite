#define FOSC	(20000000L)
#define SCI_EIGHT	(0)
#define SCI_NINE	(1)

unsigned char	sci_Init(unsigned long int, unsigned char);
void 			sci_PutByte(unsigned char);
unsigned char	sci_GetByte(void);
unsigned char	sci_GetFERR(void);
unsigned char	sci_CheckOERR(void);

