#include <common.h>
#include <serial.h>

#define SYS_CNTL_BASE      0xB0000000
#define AGPIO_CFG      (SYS_CNTL_BASE+0x3C)
#define GPIO1_MODE     (SYS_CNTL_BASE+0x60)
#define GPIO2_MODE     (SYS_CNTL_BASE+0x64)
#define GPIO0_CTRL     (SYS_CNTL_BASE+0x600)
#define GPIO1_CTRL     (SYS_CNTL_BASE+0x604)
#define GPIO0_DATA     (SYS_CNTL_BASE+0x620)
#define GPIO1_DATA     (SYS_CNTL_BASE+0x624)

#define GPIO_Power_Enable    11
#define GPIO_AC_IN           25
#define GPIO_FAST_CHARGE     37 
#define GPIO_Power_Key       24
#define GPIO_CHARGE_STATUS   40
#define LED_SYS              15
#define LED_WIFI             39
#define LED_INTERNET         41

#define RALINK_REG(x)	        (*((volatile u32 *)(x)))
#define GPIO_PWRKEY_GET()       ((RALINK_REG(GPIO0_DATA) >> GPIO_Power_Key) & 1)
#define GPIO_ACIN_GET()         ((RALINK_REG(GPIO0_DATA) >> GPIO_AC_IN) & 1)
#define LED_SYS_GET()           ((RALINK_REG(GPIO0_DATA) >> LED_SYS) & 1)
#define LED_WIFI_GET()          ((RALINK_REG(GPIO1_DATA) >> (LED_WIFI-32)) & 1)
#define LED_INTERNET_GET()      ((RALINK_REG(GPIO1_DATA) >> (LED_INTERNET-32)) & 1)
#define CHARGE_STATUS_GET()     ((RALINK_REG(GPIO1_DATA) >> (GPIO_CHARGE_STATUS-32)) & 1)

int init_gpio()
{
    unsigned int tmp_reg;

    tmp_reg = RALINK_REG(AGPIO_CFG);
    tmp_reg |= (0x1f<<16);   //set EPHY_GPIO_AIO_EN to 1111
    //tmp_reg & ~(1<<16);    //set EPHY_P0_DIS to 0
    tmp_reg |= (1<<4);        //set REF_CLKO_AIO_EN to 1
    RALINK_REG(AGPIO_CFG) = tmp_reg;  

    tmp_reg = RALINK_REG(GPIO1_MODE);    
    tmp_reg = (tmp_reg & (~(3 << 10))) | (1 << 10);           //SD_MODE=01
    tmp_reg = (tmp_reg & (~3));                               //GPIO_MODE=00
    tmp_reg = (tmp_reg & (~(3 << 2))) | (1 << 2);             //SPIS_MODE=01
    RALINK_REG(GPIO1_MODE) = tmp_reg;

    RALINK_REG(GPIO0_CTRL) &= ~(1 << GPIO_Power_Key);           
    RALINK_REG(GPIO0_CTRL) &= ~(1 << GPIO_AC_IN); 

    RALINK_REG(GPIO0_CTRL) |= (1 << GPIO_Power_Enable); 
    RALINK_REG(GPIO0_DATA) |= (1 << GPIO_Power_Enable);

    RALINK_REG(GPIO0_CTRL) |= (1 << LED_SYS); 
    //RALINK_REG(GPIO0_DATA) |= (1 << LED_SYS);
    RALINK_REG(GPIO0_DATA) &= ~(1 << LED_SYS);

    //------------------------------------------------------------------------
    tmp_reg = RALINK_REG(GPIO2_MODE);  
    tmp_reg = (tmp_reg & (~(3 << 10))) | (1 << 10);        //P4_LED_AN_MODE=01
    tmp_reg = (tmp_reg & (~(3 << 6))) | (1 << 6);          //P2_LED_AN_MODE=01
    tmp_reg = (tmp_reg & (~(3 << 8))) | (1 << 8);          //P3_LED_AN_MODE=01
    tmp_reg = (tmp_reg & (~(1 << 18))) | (1 << 18);        //REFCLK_MODE=1
    RALINK_REG(GPIO2_MODE) = tmp_reg;

    RALINK_REG(GPIO1_CTRL) |= (1 << (LED_WIFI-32)); 
    //RALINK_REG(GPIO1_DATA) |= (1 << (LED_WIFI-32));
    RALINK_REG(GPIO1_DATA) &= ~(1 << (LED_WIFI-32));    

    RALINK_REG(GPIO1_CTRL) |= (1 << (LED_INTERNET-32)); 
    //RALINK_REG(GPIO1_DATA) |= (1 << (LED_INTERNET-32));
    RALINK_REG(GPIO1_DATA) &= ~(1 << (LED_INTERNET-32)); 

    RALINK_REG(GPIO1_CTRL) &= ~(1 << (GPIO_CHARGE_STATUS-32));

    RALINK_REG(GPIO1_CTRL) |= (1 << (GPIO_FAST_CHARGE-32)); 
    RALINK_REG(GPIO1_DATA) |= (1 << (GPIO_FAST_CHARGE-32));

    return 0;
}

int is_charge_mode()
{
    return (GPIO_PWRKEY_GET() && !GPIO_ACIN_GET());
}

int no_charge_no_press()
{
    return (GPIO_ACIN_GET() && GPIO_PWRKEY_GET());
}

int charger_main()
{
    printf("\npower_key=%d, charge_status=%d, usb5vin=%d\n",  GPIO_PWRKEY_GET(),  CHARGE_STATUS_GET(),  GPIO_ACIN_GET());

    while (is_charge_mode()) {
        mdelay(1000);
        if (CHARGE_STATUS_GET()) {
            RALINK_REG(GPIO1_DATA) |= (1 << (LED_INTERNET-32));
        } else {
            if ( LED_INTERNET_GET() ) {
                //RALINK_REG(GPIO0_DATA) &= ~(1 << LED_SYS);
                RALINK_REG(GPIO1_DATA) &= ~(1 << (LED_INTERNET-32));
            } else {
                //RALINK_REG(GPIO0_DATA) |= (1 << LED_SYS);
                RALINK_REG(GPIO1_DATA) |= (1 << (LED_INTERNET-32));                
            }
        }
        mdelay(1000);
    };
    
    if (no_charge_no_press()) {
        //RALINK_REG(GPIO0_DATA) &= ~(1 << LED_SYS);
        RALINK_REG(GPIO1_DATA) &= ~(1 << (LED_WIFI-32));
        printf("\nno usb 5v in, power off\n");
        while(1) {
            mdelay(500);
            RALINK_REG(GPIO0_DATA) &= ~(1 << GPIO_Power_Enable);
        }
    }    

    //RALINK_REG(GPIO1_DATA) |= (1 << (LED_WIFI-32));    
    RALINK_REG(GPIO1_DATA) |= (1 << (LED_INTERNET-32));
    //RALINK_REG(GPIO0_DATA) |= (1 << LED_SYS);

    RALINK_REG(GPIO1_DATA) &= ~(1 << (GPIO_FAST_CHARGE-32));

    printf("\n quit fast charge mode, start boot kernel....\n");    
    return 0;
}

