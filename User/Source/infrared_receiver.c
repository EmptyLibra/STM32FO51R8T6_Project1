#include "infrared_receiver.h"

/* Detected infrared signal from different pults on NEC protocol (using Extented NEC protocol).
*  Executes commands to control LED and buttons.
*  Using infrared receiver on the PB10 pin and channel 3 of the Timer2 in capture mode.
*  Scheme of infrared receiver:
      |----------|
      |    __    |
      |   /  \   |
      |   \__/   |
      |----------|
        |  |   |
        |  |   |
       V0 GND  5V
*/

/*===========================Variables==================================*/
static uint16_t currData[64] = {0};                       // Contains pulse durations (start puls, 0 or 1)
static uint8_t currDataIndex = 0;
static uint16_t prevDataElem = 0, curDataElem = 0;


static uint32_t IRData = 0;                               // Contains current ir command like: 0x00FF9A65
static uint8_t IRDataCounter = 0;
uint16_t IR_curButton = 0;
//
/*===========================Functions==================================*/
void TIM2_IRQHandler(void){
    // pult testing
//    if(TIM_GetITStatus(TIM2, TIM_IT_CC3) == SET){
//        TIM_ClearITPendingBit(TIM2, TIM_IT_CC3);
//        currData[currDataIndex] = TIM_GetCapture3(TIM2);
//        if(currDataIndex > 0){
//            currData2[currDataIndex] = currData[currDataIndex] - currData[currDataIndex-1];
//        }
//        currDataIndex++;
//    }
    // real work
    if(TIM_GetITStatus(TIM2, TIM_IT_CC3) == SET){
        if(currDataIndex > 60){
            IRDataCounter = 0;
            currDataIndex = 0;
            IRData = 0;
        }
        TIM_ClearITPendingBit(TIM2, TIM_IT_CC3);
        curDataElem = TIM_GetCapture3(TIM2);
        if(currDataIndex > 0){
            currData[currDataIndex] = curDataElem - prevDataElem;
            prevDataElem = curDataElem;
            if((currData[currDataIndex] > 460) && (currData[currDataIndex] < 520)){
                IRData = 0;
                IRDataCounter = 0;
            } else if((currData[currDataIndex] > 100) && (currData[currDataIndex] < 125)){
                if(IRDataCounter < 32){
                    IRData = IRData << 1;
                    IRDataCounter++;
                }                
            } else if((currData[currDataIndex] > 210) && (currData[currDataIndex] < 240)){
                if(IRDataCounter < 32){
                    IRData = (IRData << 1) | 1;
                    IRDataCounter++;
                }
            }
        } else {
            currData[currDataIndex] = curDataElem;;
            prevDataElem = curDataElem;
        }
        if(IRDataCounter == 32){
            switch(IRData){
                case IR_BUTTON_TOP:
                case IF_FUNAI_BUTTON_TOP:
                    IR_curButton = BUTTON_TOP;
                    break;
                case IR_BUTTON_BOTOOM:
                case IF_FUNAI_BUTTON_BOTTOM:
                    IR_curButton = BUTTON_BOTOOM;
                    break;
                case IR_BUTTON_RIGHT:
                case IF_FUNAI_BUTTON_RIGHT:
                    IR_curButton = BUTTON_RIGHT;
                    break;
                case IR_BUTTON_LEFT:
                case IF_FUNAI_BUTTON_LEFT:
                    IR_curButton = BUTTON_LEFT;
                    break;
                case IR_BUTTON_SELECT:
                case IR_FUNAI_BUTTON_OK:
                    IR_curButton = BUTTON_SELECT;
                    break;
                case IR_BUTTON_BACK:
                case IF_FUNAI_BUTTON_BACK:
                    IR_curButton = BUTTON_BACK;
                    break;
                case IR_GREEN_LED:
                case IF_FUNAI_GREEN_LED:
                    GPIO_WriteBit(GPIOC, LD4_PIN, (BitAction)(!GPIO_ReadOutputDataBit(GPIOC, LD4_PIN)));
                    break;
                case IR_BLUE_LED:
                case IF_FUNAI_BLUE_LED:
                    GPIO_WriteBit(GPIOC, LD3_PIN, (BitAction)(!GPIO_ReadOutputDataBit(GPIOC, LD3_PIN)));
                    break;
                default:
                    IR_curButton = 0;
            }
            IRDataCounter = 0;
            currDataIndex = 0;
            IRData = 0;
        } else{
            currDataIndex++;
        }
    }
}

void IR_init(){
    RCC_AHBPeriphClockCmd(RCC_AHBENR_GPIOBEN, ENABLE);
    GPIO_InitTypeDef GPIOx_ini;
    
    GPIOx_ini.GPIO_Pin = IR_PIN;
    GPIOx_ini.GPIO_Mode = GPIO_Mode_AF;
    GPIOx_ini.GPIO_Speed = GPIO_Speed_2MHz;   // 2, 10, 50 ̃ö
    GPIOx_ini.GPIO_OType = GPIO_OType_PP;
    GPIOx_ini.GPIO_PuPd = GPIO_PuPd_DOWN;
    GPIO_Init(IR_PORT, &GPIOx_ini);
    
    // PB10 - AF2 - TIM2_CH3
    GPIO_PinAFConfig(IR_PORT, IR_PIN_SOURCE, GPIO_AF_2);
    
    // timer2 settings
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    TIM_TimeBaseInitTypeDef TIM2_ini;
    
    TIM2_ini.TIM_Prescaler =  480 - 1;
    TIM2_ini.TIM_CounterMode = TIM_CounterMode_Up;
    TIM2_ini.TIM_Period = 0xFFFF;
    TIM2_ini.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInit(TIM2, &TIM2_ini);
    
    // timer's chanell settings
    TIM_ICInitTypeDef TIM2_ICinit;
    TIM2_ICinit.TIM_Channel = TIM_Channel_3;
    TIM2_ICinit.TIM_ICFilter = 4;
    TIM2_ICinit.TIM_ICPolarity = TIM_ICPolarity_Rising;
    TIM2_ICinit.TIM_ICPrescaler = TIM_ICPSC_DIV1;
    TIM2_ICinit.TIM_ICSelection = TIM_ICSelection_DirectTI;
    TIM_ICInit(TIM2, &TIM2_ICinit);

    // enable interrupts
    TIM_ITConfig(TIM2, TIM_IT_CC3, ENABLE);
    TIM_ClearFlag(TIM2, TIM_FLAG_CC3);
    TIM_Cmd(TIM2, ENABLE);             // enable timer 2
    
    NVIC_EnableIRQ(TIM2_IRQn);
}

