int disable_interrupts (void) 
{ 
	unsigned long old,temp; 
	__asm__ __volatile__("mrs %0, cpsr\n" 
			"orr %1, %0, #0x80\n" 
			"msr cpsr_c, %1" 
			: "=r" (old), "=r" (temp) 
			: 
			: "memory"); 
	return (old & 0x80) == 0; 
} 

int main()
{
	return disable_interrupts();
}
