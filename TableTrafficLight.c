// ***** 0. Documentation Section *****
// TableTrafficLight.c
// Runs on LM4F120/TM4C123
// Index implementation of a Moore finite state machine to operate a traffic light.  

// east/west red light connected to PB5
// east/west yellow light connected to PB4
// east/west green light connected to PB3
// north/south facing red light connected to PB2
// north/south facing yellow light connected to PB1
// north/south facing green light connected to PB0
// pedestrian detector connected to PE2 (1=pedestrian present)
// north/south car detector connected to PE1 (1=car present)
// east/west car detector connected to PE0 (1=car present)
// "walk" light connected to PF3 (built-in green LED)
// "don't walk" light connected to PF1 (built-in red LED)

// ***** 1. Pre-processor Directives Section *****
#include "TExaS.h"
#include "tm4c123gh6pm.h"


#include "PLL.h"
#include "SysTick.h"

#define TRAFFIC_LIGHTS           /*GPIO_PORTB_DATA_R*/ (*((volatile unsigned long *)0x400050FC)) //Port B out, bits 5-0  1111 11
#define WALK_LIGHTS		           GPIO_PORTF_DATA_R //Port F out, bits 3,1  
#define SENSORS 	         			 (*((volatile unsigned long *)0x4002401C)) //Port E in, bits 2-0
	
	
// ***** 2. Global Declarations Section *****

// FUNCTION PROTOTYPES: Each subroutine defined
void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts

// ***** 3. Subroutines Section *****
// Linked data structure
struct State {
  unsigned long TrafficOut; 
  unsigned long WalkOut; 
  unsigned long Time;  
  unsigned long Next[8];}; 
typedef const struct State STyp;
	
//Outputs are PB		1			1				1			1			1			1
//									WRed 	WYell		wGre	sRed	sYell	sGre
	
//Outputs are PF		1						0		1					0
//									WalkGreen		x		WalkRed		x
	
//Sensors are 0x0		1			 1					1 =
//						0x0 | Walk | Car-West | Car-South
#define carGoWest    	0
#define carWaitWest  	1
#define carStopWest  	2
#define carGoSouth   	3
#define carWaitSouth 	4
#define carStopSouth 	5
#define walk 					6
#define walkWaitOn1 	7
#define walkWaitOff1 	8
#define walkWaitOn2 	9
#define walkWaitOff2 	10
#define noWalk 				11
STyp FSM[12]={
								//000				001						010					011						100						101						110						111
 {0x0C,0x2,500,{carGoWest,	carWaitWest,	carGoWest,	carWaitWest,	carWaitWest,	carWaitWest,	carWaitWest,	carWaitWest,}}, //Car Go West
 {0x14,0x2,500,{carStopWest,carStopWest,	carStopWest,carStopWest,	carStopWest,	carStopWest,	carStopWest,	carStopWest,	}},	//Car Wait West
 {0x24,0x2,500,{carGoWest,	carGoSouth,		carGoWest,	carGoSouth,		walk,					carGoSouth,		walk,					carGoSouth,	}},	//Car Stop West
 {0x21,0x2,500,{carGoSouth,carGoSouth,		carWaitSouth,carWaitSouth,carWaitSouth,	carWaitSouth,	carWaitSouth,	carWaitSouth,}},//Car Go South
 {0x22,0x2,500,{carStopSouth,carStopSouth,carStopSouth,carStopSouth,carStopSouth,carStopSouth,	carStopSouth,	carStopSouth,}},//Car Wait South
 {0x24,0x2,500,{carGoWest,	carGoSouth,		carGoWest,	carGoWest,		walk,					walk,					walk,					walk,	}},				//Car Stop South
 {0x24,0x8,500,{walk,			walkWaitOn1,	walkWaitOn1,walkWaitOn1,		walk,					walkWaitOn1,	walkWaitOn1,	walkWaitOn1,}},	//Walk
 {0x24,0x2,100,{walk,			walkWaitOff1,	walkWaitOff1,walkWaitOff1,	walk,					walkWaitOff1,	walkWaitOff1,	walkWaitOff1,}},//Walk Wait On 1
 {0x24,0x0,100,{walk,			walkWaitOn2,	walkWaitOn2,walkWaitOn2,		walk,					walkWaitOn2,	walkWaitOn2,	walkWaitOn2,}},	//Walk Wait Off 1
 {0x24,0x2,100,{walk,			walkWaitOff2,	walkWaitOff2,walkWaitOff2,	walk,					walkWaitOff2,	walkWaitOff2,	walkWaitOff2,}},//Walk Wait On 2
 {0x24,0x0,100,{walk,			noWalk,				noWalk,			noWalk,					walk,					noWalk,				noWalk,				noWalk,}},			//Walk Wait Off 2
 {0x24,0x2,500,{noWalk,		carGoSouth,		carGoWest,	carGoWest,			walk,					carGoSouth,		carGoWest,		carGoWest,}},	//No Walk
 
 };
unsigned long S;  // index to the current state 
unsigned long Input; 
 
 
 
int main(void){ 
  TExaS_Init(SW_PIN_PE210, LED_PIN_PB543210); // activate grader and set system clock to 80 MHz
 
  
  EnableInterrupts();
  while(1){
     volatile unsigned long delay;
     volatile unsigned long totalInput;
		PLL_Init();       // 80 MHz, Program 10.1
		SysTick_Init();   // Program 10.2
		SYSCTL_RCGC2_R |= 0x32;      // 1) B E F
		delay = SYSCTL_RCGC2_R;      // 2) no need to unlock
		
		GPIO_PORTE_AMSEL_R &= ~0x07; // 3) disable analog function on PE2-0
		GPIO_PORTE_PCTL_R &= ~0x00FFFFFF; // 4) enable regular GPIO
		GPIO_PORTE_DIR_R &= ~0x07;   // 5) inputs on PE2-0
		GPIO_PORTE_AFSEL_R &= ~0x07; // 6) regular function on PE2-0
		GPIO_PORTE_DEN_R |= 0x07;    // 7) enable digital on PE2-0
		
		GPIO_PORTB_AMSEL_R &= ~0x3F; // 3) disable analog function on PB5-0
		GPIO_PORTB_PCTL_R &= ~0x00FFFFFF; // 4) enable regular GPIO
		GPIO_PORTB_DIR_R |= 0x3F;    // 5) outputs on PB5-0
		GPIO_PORTB_AFSEL_R &= ~0x3F; // 6) regular function on PB5-0
		GPIO_PORTB_DEN_R |= 0x3F;    // 7) enable digital on PB5-0
		
		GPIO_PORTF_AMSEL_R &= ~0x0A; // 3) disable analog function on PF1,3
		GPIO_PORTF_PCTL_R &= ~0x00FFFFFF; // 4) enable regular GPIO
		GPIO_PORTF_DIR_R |= 0x0A;    // 5) outputs on PF1,3
		GPIO_PORTF_AFSEL_R &= ~0x0A; // 6) regular function on PF1,3
		GPIO_PORTF_DEN_R |= 0x0A;    // 7) enable digital on PF1,3
		
		S = carGoWest;  
		while(1){
			TRAFFIC_LIGHTS = FSM[S].TrafficOut;  // set traffic lights
			WALK_LIGHTS = FSM[S].WalkOut;  			// set walk lights
			
			SysTick_Wait10ms(FSM[S].Time/10);
			Input = SENSORS;     // read sensors
			
			//Swap west and south as they're the wrong way round !
			Input = (Input&~3)|(Input&1)<<1|(Input&2)>>1;
			
			//Output !			
			S = FSM[S].Next[Input];  
		}
  }
}

 