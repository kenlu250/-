//無源蜂鳴器或喇叭
#include <SPI.h>
#include <Wire.h> 
#include <MFRC522.h>
#include <Keypad.h>
#include <LiquidCrystal_I2C.h>

#define KEY_ROWS 4  // 薄膜按鍵的列數
#define KEY_COLS 4  // 薄膜按鍵的行數
#define LCD_ROWS 2  // LCD顯示器的列數
#define LCD_COLS 16 // LCD顯示器的行數
// 設置按鍵模組
char keymap[KEY_ROWS][KEY_COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
 
byte rowPins[KEY_ROWS] = {9, 8, 7, 6}; // 8,7,6,5
byte colPins[KEY_COLS] = {5, 4, 3, 2}; //4,3,2,1


Keypad keypad = Keypad(makeKeymap(keymap), rowPins, colPins, KEY_ROWS, KEY_COLS);
 
String passcode = "4321";   // 預設密碼
String inputCode = "";      // 暫存用戶的按鍵字串
bool acceptKey = true;      // 代表是否接受用戶按鍵輸入的變數，預設為「接受」
 
LiquidCrystal_I2C lcd(0x27,16,2);  // set the LCD address to 0x27 for a 16 chars and 2 line display

const int speaker=A1;  //蜂鳴器
const int Rled=A2;    //紅燈
const int Gled=A3;    //綠燈,同時控制大門電磁鎖
const int RST_PIN=A0;  
const int SS_PIN=10;  
int i;
int size;

char str1[]="Input password..";
char str2[]="PIN:";
char str3[]="Welcome home!";
char str4[]="***WRONG!!***";
char str5[]="RFID tag is:";



MFRC522 rfid(SS_PIN,RST_PIN); 
int index=0;          //卡號索引
const int number=4;   //四個會員
int serialNum=-1;     //會員編號,由0開始
byte card[number][4]={{0x59,0x74,0xF6,0x62},{0x79,0xEA,0xE9,0x84},{0xD9,0xDC,0xF0,0x5E},{0x5D,0x2F,0xEA,0x84}}; //會員卡號,依卡號自行更改輸入
void compTag(void);   //比較函數

void clearRow(byte n) {
  byte last = LCD_COLS - n;
  lcd.setCursor(n, 1); // 移動到第2行，"PIN:"之後
 
  for (byte i = 0; i < last; i++) {
    lcd.print(" ");
  }
  lcd.setCursor(n, 1);
}
 
// 顯示「歡迎光臨」後，重設LCD顯示文字和輸入狀態。
void resetLocker() {
  lcd.clear();
  size=sizeof(str1);
  printStr(size,str1);
  lcd.setCursor(0, 1);  // 切換到第2行
  size=sizeof(str2);
  printStr(size,str2);
  lcd.cursor();
 
  acceptKey = true;
  inputCode = "";
}
 
// 比對用戶輸入的密碼
void checkPinCode() {
  acceptKey = false;  // 暫時不接受用戶按鍵輸入
  clearRow(0);        // 從第0個字元開始清除LCD畫面
  lcd.noCursor();
  lcd.setCursor(0, 1);  // 切換到第2行
  // 比對密碼
  if (inputCode == passcode) {
      size=sizeof(str3);
      printStr(size,str3);
      tone(speaker,1000);       
      delay(200);
      noTone(speaker);
  } else {
      size=sizeof(str4);
      printStr(size,str4);
      for(i=0;i<2;i++) {        //產生兩短嗶聲
      tone(speaker,1000);
      delay(50);
      noTone(speaker);
      delay(50); 
    }    
  }
  delay(3000);
  resetLocker();     // 重設LCD顯示文字和輸入狀態
}

void printStr(int size,char *str)
{
  for(int i=0;i<size-1;i++)
  {
     lcd.print(str[i]); 
  }  
} 

void setup()          //設定初值
{
   Serial.begin(9600);        //初始化序列埠
   SPI.begin();
   rfid.PCD_Init();
   lcd.init();
   lcd.backlight();
   //resetLocker();
   lcd.setCursor(0,0);
   size=sizeof(str5);
   printStr(size,str5);
   pinMode(Gled,OUTPUT);     //設定D8為輸出埠
   pinMode(Rled,OUTPUT);     //設定D9為輸出埠
}






void loop()     //迴圈函數
{
  if(rfid.PICC_IsNewCardPresent())
  {  
    if(rfid.PICC_ReadCardSerial()) 
    {  
      lcd.setCursor(0,1);      
      int size=rfid.uid.size;
      for(i=0;i<size;i++)
      {
        Serial.print(rfid.uid.uidByte[i],HEX);  
        Serial.print(" ");          
        lcd.print(rfid.uid.uidByte[i]/16,HEX);   
        lcd.print(rfid.uid.uidByte[i]%16,HEX);
        lcd.print(" ");     
      }
      Serial.println("");  
      compTag();                    //比較已讀取的卡號是否為會員?
    } 
    rfid.PICC_HaltA();  
    delay(1000);

  if(serialNum>=0)            //是會員?
  {
    resetLocker();
    char key = NO_KEY;
    while(key == NO_KEY ){
       key = keypad.getKey();
    }
     // 若目前接受用戶輸入，而且有新的字元輸入…
  
    while(key != '#'){   
      if (key == '*') {   // 清除畫面
         clearRow(4);  // 從第4個字元開始清除
         inputCode = "";
       } else {
         inputCode += key;  // 儲存用戶的按鍵字元
         lcd.print('*');
       }
       key = NO_KEY;
       while(key == NO_KEY ){
       key = keypad.getKey();
      }
    }
    checkPinCode();   // 比對輸入密碼 
  }
    lcd.clear();
    size=sizeof(str5);
    printStr(size,str5);
  }
}





void compTag(void)    //比較函數
{
  int exact;          //旗標
  int i,j;
  serialNum=-1;
  for(i=0;i<number;i++)       //比對所有會員卡號
  {
    exact=1;                  //先假設所讀取的卡號是會員
    for(j=0;j<4;j++)          //4位會員卡號為
    {
      if(rfid.uid.uidByte[j]!=card[i][j])  //卡號相同?
        exact=0;              //卡號不同，設定exact=0
    }
    if(exact==1)              //卡號相同?
       serialNum=i;           //記錄會員編號
  }  
  if(serialNum>=0)            //是會員?
  {
    digitalWrite(Gled,HIGH);  //綠燈閃爍一次，產生一長嗶聲
    digitalWrite(Rled,LOW);   //紅燈不亮
    tone(speaker,1000);       
    delay(200);               
    digitalWrite(Gled,LOW);    
    digitalWrite(Rled,LOW);
    noTone(speaker); 
  }  
  else                        //不是會員
  {
    for(i=0;i<2;i++)          //紅燈閃爍兩次，產生兩短嗶聲
    {
      digitalWrite(Gled,LOW); //綠燈不亮
      digitalWrite(Rled,HIGH);
      tone(speaker,1000);
      delay(50);
      digitalWrite(Gled,LOW);
      digitalWrite(Rled,LOW);
      noTone(speaker);
      delay(50); 
    }    
  }  
} 
