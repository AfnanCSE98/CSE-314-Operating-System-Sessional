#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "traps.h"
#include "spinlock.h"

#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"
#define RESET   "\x1b[0m"


// Interrupt descriptor table (shared by all CPUs).
struct gatedesc idt[256];
extern uint vectors[];  // in vectors.S: array of 256 entry pointers
struct spinlock tickslock;
uint ticks;

int NRU = 0;

void
tvinit(void)
{
  int i;

  for(i = 0; i < 256; i++)
    SETGATE(idt[i], 0, SEG_KCODE<<3, vectors[i], 0);
  SETGATE(idt[T_SYSCALL], 1, SEG_KCODE<<3, vectors[T_SYSCALL], DPL_USER);

  initlock(&tickslock, "time");
}

void
idtinit(void)
{
  lidt(idt, sizeof(idt));
}
//--------------------------added by afnan-------------------------


static pte_t *
walkpgdir(pde_t *pgdir, const void *va, int alloc)
{
  pde_t *pde;
  pte_t *pgtab;

  pde = &pgdir[PDX(va)];
  if(*pde & PTE_P){
    pgtab = (pte_t*)P2V(PTE_ADDR(*pde));
  } else {
    if(!alloc || (pgtab = (pte_t*)kalloc()) == 0)
      return 0;
    // Make sure all those PTE_P bits are zero.
    memset(pgtab, 0, PGSIZE);
    // The permissions here are overly generous, but they can
    // be further restricted by the permissions in the page table
    // entries, if necessary.
    *pde = V2P(pgtab) | PTE_P | PTE_W | PTE_U;
    lcr3(V2P(myproc()->pgdir));
  }
  return &pgtab[PTX(va)];
}



void print_page_fault_info(char* va){
    cprintf(RED "\nT_PGFLT\n" RESET);
    cprintf("rcr2 = %d\n", va);
    va = (char*) PGROUNDDOWN((int)va);
    cprintf("va = %d\n", va);
}
/*
int move_swap_page_to_dummy_position(char* va , int *swapIdx , char* swapVa){
    *swapIdx = -1;
    for(int i=0; i<MAX_PSYC_PAGES ; i++){
        if(myproc()->swapPageArray[i].virtual_Address == va && myproc()->swapPageArray[i].used==1){
            *swapIdx = i;break;
        }
    }
    if(*swapIdx == -1){
      panic("Pagefault occured but page not found in swapFile\n");
    }

    swapVa = myproc()->swapPageArray[*swapIdx].virtual_Address;

    int tmp_swapIdx = MAX_PSYC_PAGES;
    for(int i=0 ; i<MAX_PSYC_PAGES ; i++){
        if(myproc()->swapPageArray[i].virtual_Address == (char*)0xffffffff && myproc()->swapPageArray[i].used==0){
            tmp_swapIdx = i;break;
        }
    }

    char buffer[PGSIZE/4];

    for(int i=0 ; i<4 ; i++){
        readFromSwapFile(myproc(), buffer, (*swapIdx)*PGSIZE + i*PGSIZE/4, PGSIZE/4);
        writeToSwapFile(myproc(), buffer, tmp_swapIdx*PGSIZE + i*PGSIZE/4, PGSIZE/4);
    }

    cprintf("Swap page moved to a dummy page slot\n");

    return tmp_swapIdx;
}
*/
void move_memPage_to_swapFile_NRU(int swapIdx){
      int nruIdx = -1;
      int found = 0;
      //case 1 : not referenced and not modified
      for(int i=0 ; i<MAX_PSYC_PAGES ; i++){
          pte_t * pte_NRU = walkpgdir(myproc()->pgdir 
                           , myproc()->memPageArray[i].virtual_Address , 0);
          
          if((*pte_NRU&PTE_A)==0 && (*pte_NRU&PTE_M)==0){
              found=1;nruIdx = i;break;
            }   
      }
      //case 2 : not referenced and modified
      if(found == 0){
          for(int i=0 ; i<MAX_PSYC_PAGES ; i++){
          pte_t * pte_NRU = walkpgdir(myproc()->pgdir 
                             , myproc()->memPageArray[i].virtual_Address , 0);
          
            if((*pte_NRU&PTE_A)==0 && (*pte_NRU&PTE_M)!=0){
              found=1;nruIdx = i;break;
            }   
          } 
      }
      //case 3 : referenced and not modified
      if(found == 0){
          for(int i=0 ; i<MAX_PSYC_PAGES ; i++){
              pte_t * pte_NRU = walkpgdir(myproc()->pgdir 
                             , myproc()->memPageArray[i].virtual_Address , 0);
          
              if((*pte_NRU&PTE_A)!=0 && (*pte_NRU&PTE_M)==0){
                  found=1;nruIdx = i;break;
              }   
          }
      }
      //case 4 : referenced and modified
      if(found == 0){
          for(int i=0 ; i<MAX_PSYC_PAGES ; i++){
            pte_t * pte_NRU = walkpgdir(myproc()->pgdir 
                             , myproc()->memPageArray[i].virtual_Address , 0);
          
            if((*pte_NRU&PTE_A)!=0 && (*pte_NRU&PTE_M)!=0){
              found=1;nruIdx = i;break;
            }   
          }
      }

    char *headVa = myproc()->memPageArray[nruIdx].virtual_Address;
     
    myproc()->memPageArray[nruIdx].virtual_Address = (char*)0xffffffff;//setting invalid va
    myproc()->memPageArray[nruIdx].used = 0;
    myproc()->memPageArray[nruIdx].nextIdx = -1;
    myproc()->headOfQueueIdx = myproc()->memPageArray[nruIdx].nextIdx;//moving head pointer forward
    
    //writing to swapFile
    writeToSwapFile(myproc(), headVa, swapIdx*PGSIZE, PGSIZE);
    
    myproc()->swapPageArray[swapIdx].used = 1;
    myproc()->swapPageArray[swapIdx].virtual_Address = headVa;

    //free headVa
    //pte_t *tmp_head = (pte_t*)walkpgdir(myproc()->pgdir, headVa, 0);
    //kfree((char*)PTE_ADDR(P2V(tmp_head)));
    kfree((char*)PTE_ADDR(P2V(*walkpgdir(myproc()->pgdir, headVa, 0)))); 

    //updating flags
    pte_t *pteHead = (pte_t*)walkpgdir(myproc()->pgdir, headVa, 0);
    *pteHead = (*pteHead|PTE_PG)&(~PTE_P);
    lcr3(V2P(myproc()->pgdir));

    cprintf("Page from memory moved to swapFile using NRU\n");

}

void move_memPage_to_swapFile_FIFO(int swapIdx){
    int headIdx = myproc()->headOfQueueIdx;
    if(headIdx == -1){
      panic("Queue error inside pagefault");
    }

    //getting va of the page that will be swapped out
    char *headVa = myproc()->memPageArray[headIdx].virtual_Address;

    myproc()->memPageArray[headIdx].virtual_Address = (char*)0xffffffff;//setting invalid va
    myproc()->memPageArray[headIdx].used = 0;
    myproc()->memPageArray[headIdx].nextIdx = -1;
    myproc()->headOfQueueIdx = myproc()->memPageArray[headIdx].nextIdx;//moving head pointer forward
    
    //writing to swapFile
    writeToSwapFile(myproc(), headVa, swapIdx*PGSIZE, PGSIZE);
    
    myproc()->swapPageArray[swapIdx].used = 1;
    myproc()->swapPageArray[swapIdx].virtual_Address = headVa;

    //free headVa
    //pte_t *tmp_head = (pte_t*)walkpgdir(myproc()->pgdir, headVa, 0);
    //kfree((char*)PTE_ADDR(P2V(tmp_head)));
    kfree((char*)PTE_ADDR(P2V(*walkpgdir(myproc()->pgdir, headVa, 0)))); 

    //updating flags
    pte_t *pteHead = (pte_t*)walkpgdir(myproc()->pgdir, headVa, 0);
    *pteHead = (*pteHead|PTE_PG)&(~PTE_P);
    lcr3(V2P(myproc()->pgdir));

    cprintf("Page from memory moved to swapFile Using FIFO\n");

}

void move_swap_page_to_memory(char* swapVa , int tmp_swapIdx){
    char *mem = kalloc();   //new memory acquired
    if(mem == 0){
      panic("page fault and memory out");
    }

    int idx = -1;
    for(int i=0; i<MAX_PSYC_PAGES; i++){
      if(myproc()->memPageArray[i].used == 0){
        idx = i;
        break;
      }
    }
    if(idx == -1){
      panic("pgfault and no free slot in memory\n");
    }
    //idx = location where swapPage will be moved to

     //swapPage to be moved to memory
    pte_t *pteSwap;
    pteSwap = (pte_t*)walkpgdir(myproc()->pgdir, swapVa, 0);
    *pteSwap = ((((V2P(mem))>>12)<<12) |PTE_P|PTE_W|PTE_U ) ;//since it's going to memory , so enable these flags
    lcr3(V2P(myproc()->pgdir));

    int tail = myproc()->headOfQueueIdx;   
    //empty queue
    if(tail == -1){  
        myproc()->headOfQueueIdx = idx;
    }
    else{
      while(myproc()->memPageArray[tail].nextIdx != -1){
          tail = myproc()->memPageArray[tail].nextIdx;
      }
      //setting tail point to the newly added page
      myproc()->memPageArray[tail].nextIdx = idx;
    }

    //updating memPage to contain swapPage
    myproc()->memPageArray[idx].nextIdx = -1;
    myproc()->memPageArray[idx].used = 1;
    myproc()->memPageArray[idx].virtual_Address = swapVa;

    char buffer[PGSIZE/4];
    memset(buffer , 0 ,PGSIZE/4);
    int j;
    //cprintf("luper age\n");
    for(int i=0 ; i<4; i++){
      j = readFromSwapFile(myproc(), buffer, tmp_swapIdx*PGSIZE + i*PGSIZE/4, PGSIZE/4);
      if(j==-1){
        panic("While moving swapPage to memory : Couldn't readd from swapFile");
      }
      //cprintf("memmove er age\n");
      char* dst = memmove(swapVa+i*PGSIZE/4, buffer, PGSIZE/4);
      if(dst != (char*)swapVa+i*PGSIZE/4){
        panic("memove not working properly");
      }
      memset(buffer , 0 ,PGSIZE/4);
      //cprintf("memmove er pore\n");
    }
    //cprintf("luper pore\n");

   

    cprintf(GREEN "swap page finally moved to memory\n" RESET);
}
//------------------------------------------------------------------
//PAGEBREAK: 41
void
trap(struct trapframe *tf)
{
  if(tf->trapno == T_SYSCALL){
    if(myproc()->killed)
      exit();
    myproc()->tf = tf;
    syscall();
    if(myproc()->killed)
      exit();
    return;
  }

  switch(tf->trapno){
  case T_IRQ0 + IRQ_TIMER:
    //------------------------added by afnan-------------------
    if(NRU){
      reset_R();
    }
    //---------------------------------------------------------
    if(cpuid() == 0){
      acquire(&tickslock);
      ticks++;
      wakeup(&ticks);
      release(&tickslock);
    }
    lapiceoi();
     
    break;
  case T_IRQ0 + IRQ_IDE:
    ideintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE+1:
    // Bochs generates spurious IDE1 interrupts.
    break;
  case T_IRQ0 + IRQ_KBD:
    kbdintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_COM1:
    uartintr();
    lapiceoi();
    break;
  case T_IRQ0 + 7:
  case T_IRQ0 + IRQ_SPURIOUS:
    cprintf("cpu%d: spurious interrupt at %x:%x\n",
            cpuid(), tf->cs, tf->eip);
    lapiceoi();
    break;
  //---------------added by afnan----------------------//
  //pagefault case
  case T_PGFLT:
      //CR2 Contains a value called Page Fault Linear Address (PFLA).
      //When a page fault occurs, the address the program attempted to
      // access is stored in the CR2 register.
      if(myproc()->pid==1){
        break;
      }

      char* addr = (char*)rcr2();
      print_page_fault_info(addr);
      char* va = (char*) PGROUNDDOWN((int)addr);

      //getting pgtab entry
      pte_t *pte1 = (pte_t*)walkpgdir(myproc()->pgdir, va, 0);

      //if its in swap file
      if((*pte1 & PTE_P)==0 && (*pte1 & PTE_PG)!=0){
        /*
        3 steps to perform swap out andd swap in
        //----------(1) move the swap page to a dummy swap index
        //              located at another position in swap file
        //              so that it doesn't get overwritten.
        //
        //----------(2) select which page to move from memory.
        //              Then move that page to the swapFile in the 
        //              actual location of swap page.
        //
        //----------(3) Now move the swap page , located at a dummy 
        //              position , to the selected position of memory  
        */
          
          //===========step 1===================
          int swapIdx = -1;
            
          for(int i=0; i<MAX_PSYC_PAGES ; i++){
              if(myproc()->swapPageArray[i].virtual_Address == va && myproc()->swapPageArray[i].used==1){
                  swapIdx = i;break;
              }
          }
          if(swapIdx == -1){
            panic("Pagefault occured but page not found in swapFile\n");
          }

          char* swapVa = myproc()->swapPageArray[swapIdx].virtual_Address;

          int tmp_swapIdx = MAX_PSYC_PAGES;
          for(int i=0 ; i<MAX_PSYC_PAGES ; i++){
              if(myproc()->swapPageArray[i].virtual_Address == (char*)0xffffffff && myproc()->swapPageArray[i].used==0){
                  tmp_swapIdx = i;break;
              }
          }

          char buffer[PGSIZE/4];
          memset(buffer , 0 , PGSIZE/4);

          for(int i=0 ; i<4 ; i++){
              readFromSwapFile(myproc(), buffer, (swapIdx)*PGSIZE + i*PGSIZE/4, PGSIZE/4);
              writeToSwapFile(myproc(), buffer, tmp_swapIdx*PGSIZE + i*PGSIZE/4, PGSIZE/4);
              memset(buffer , 0 , PGSIZE/4);
          }

          cprintf("Swap page moved to a dummy page slot\n");
          
          //===================step 2============================
          if(NRU){
            move_memPage_to_swapFile_NRU(swapIdx);
          }
          else{
            move_memPage_to_swapFile_FIFO(swapIdx);
          }
          
          //====================step3==============================
          move_swap_page_to_memory(swapVa , tmp_swapIdx);
      }
      return;

  //----------------------------------------------------
  //PAGEBREAK: 13
  default:
    if(myproc() == 0 || (tf->cs&3) == 0){
      // In kernel, it must be our mistake.
      cprintf("unexpected trap %d from cpu %d eip %x (cr2=0x%x)\n",
              tf->trapno, cpuid(), tf->eip, rcr2());
      panic("trap");
    }
    // In user space, assume process misbehaved.
    cprintf("pid %d %s: trap %d err %d on cpu %d "
            "eip 0x%x addr 0x%x--kill proc\n",
            myproc()->pid, myproc()->name, tf->trapno,
            tf->err, cpuid(), tf->eip, rcr2());
    myproc()->killed = 1;
  }

  // Force process exit if it has been killed and is in user space.
  // (If it is still executing in the kernel, let it keep running
  // until it gets to the regular system call return.)
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
    exit();

  // Force process to give up CPU on clock tick.
  // If interrupts were on while locks held, would need to check nlock.
  if(myproc() && myproc()->state == RUNNING &&
     tf->trapno == T_IRQ0+IRQ_TIMER)
    yield();

  // Check if the process has been killed since we yielded
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
    exit();
}
