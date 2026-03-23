#include "stm32f10x.h"
#include "sys.h" 
#define TRIG PAout(3) //输出端口
#define ECHO PAin(2)  //输入端口
int overcount=0;      //记录定时器溢出次数
int length;

float Pitch,Roll,Yaw;						//角度
short gyrox,gyroy,gyroz;				//陀螺仪--角速度
short aacx,aacy,aacz;						//加速度
int Encoder_Left,Encoder_Right;	//编码器数据（速度）

int PWM_MAX=7200,PWM_MIN=-7200;	//PWM限幅变量
int MOTO1,MOTO2;								//电机装载变量

extern int Vertical_out,Velocity_out,Turn_out;
void TIM3_Int_Init()
{
		GPIO_InitTypeDef GPIO_InitStruct;
		TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
		NVIC_InitTypeDef NVIC_InitStructure;

		RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE); //时钟使能
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
			
		
		GPIO_InitStruct.GPIO_Mode=GPIO_Mode_Out_PP;//设置为推挽输出
		GPIO_InitStruct.GPIO_Pin=GPIO_Pin_3;
		GPIO_InitStruct.GPIO_Speed=GPIO_Speed_50MHz;
		GPIO_Init(GPIOA,&GPIO_InitStruct);

		GPIO_InitStruct.GPIO_Mode=GPIO_Mode_IN_FLOATING;
		GPIO_InitStruct.GPIO_Pin=GPIO_Pin_2;
		GPIO_Init(GPIOA,&GPIO_InitStruct);
	
		//定时器3初始化
		TIM_TimeBaseStructure.TIM_Period = 999; //ARR
		TIM_TimeBaseStructure.TIM_Prescaler =7199; //PSC
		TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
		TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; //向上计数模式
		TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

		TIM_ITConfig(TIM3,TIM_IT_Update,ENABLE );//使能制定TIM3中断，允许更新中断
		//中断设置
		NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
		NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;
		NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
		NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;
		NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
		NVIC_Init(&NVIC_InitStructure);

		TIM_Cmd(TIM3, DISABLE);
}
//测距函数
int Senor_Using(void) 
{
    unsigned int tim;
    unsigned int len;
    u16 cnt_i=0;
    
    TRIG=1;          
    delay_us(20);    
    TRIG=0;          
    
    // 等待回响
    while(ECHO==0)
    {
        cnt_i++;
        delay_us(20);
        if(cnt_i>2000) return 300; // 超时退出，防止死机
    } 
    
    TIM_Cmd(TIM3,ENABLE);
    while(ECHO==1);	
    TIM_Cmd(TIM3,DISABLE);    
    
    tim=TIM_GetCounter(TIM3);         
    len=(tim*100)/58.0; 
    
    TIM3->CNT=0; 
    
    // 删除了死延时，限制最大距离为 300cm
    if(len > 300 || len == 0) len = 300; 
    
    return len; 
}

int main(void)	
{
	delay_init();
	NVIC_Config();
	uart1_init(115200);	
	
	//一、//
	/**************************************************/
	uart3_init(9600);//先9600  

	OLED_Init();
	OLED_Clear();
	
	MPU_Init();
	mpu_dmp_init();
	MPU6050_EXTI_Init();
	
	Encoder_TIM2_Init();
	Encoder_TIM4_Init();
	Motor_Init();
	PWM_Init_TIM1(0,7199);
	
	TIM3_Int_Init();
	OLED_ShowString(0,3,"jiao du:",12);
	OLED_ShowString(0,4,"ju li:",12);
	OLED_ShowString(0,5,"sd:",12);
  while(1)	
	{
		
		length = Senor_Using(); //调用测距函数	
		OLED_Float(3,70,Pitch,1);
		OLED_Float(4,64,length,1);
		OLED_Num3(8,5,(int)((Encoder_Left+Encoder_Right)*2.38));		
	} 	
}

void TIM3_IRQHandler(void)
{
		if (TIM_GetITStatus(TIM3,TIM_IT_Update)!= RESET) //检查是否发生TIM3中断
		{
			TIM_ClearITPendingBit(TIM3, TIM_IT_Update );   //清楚中断更新标志
			overcount++;
		}
}
