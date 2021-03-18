/*Pass Up Vector*/
typedef struct passupvector { 
	unsigned int tlb_refill_handler;
	unsigned int tlb_refill_stackPtr;
	unsigned int exception_handler;
	unsigned int exception_stackPtr; } 
passupvector_t;


HIDDEN void iniPassUPvector(passupvector_t *puvec){
    puvec->tlbrefllhandler = (memaddr) uTLBRefillHandler;
    puvec->tlb_refill_stackPtr = KERNELSTACK;
    puvec->exceptionhandler = (memaddr) fooBar;
    puvec->exception_stackPtr = KERNELSTACK;
}
