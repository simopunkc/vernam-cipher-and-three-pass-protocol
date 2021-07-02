#include <SPI.h>
#include <RF24.h>
#include <SoftwareSerial.h>
RF24 radio(9,10);
const uint64_t pipe=0xE8E8F0F0E1LL;
SoftwareSerial mySerial(2,3);
String kunciTPP="";
String input="";
void setup(){
  Serial.begin(57600);
  mySerial.begin(57600);
  radio.begin();
  delay(100);
  radio.setDataRate(RF24_250KBPS);
  radio.setRetries(15,15);
  radio.setPALevel(RF24_PA_MIN);
  radio.openWritingPipe(pipe);
  radio.openReadingPipe(1,pipe);
  radio.startListening();
  randomSeed(analogRead(0));
}
void loop(){ 
  if(mySerial.available()){
    char inputSerial=(char)mySerial.read();
    input=input+inputSerial;
  }
  if(input.length()==8){
    if(kunciTPP==""){
      pengirimTPP();
    }
    KirimData(input);
    input="";
  }
  if(radio.available()){
    if(kunciTPP==""){
      penerimaTPP();
    }
    TerimaData();
  }
}
String vernamCipher(String biner1,String biner2){
  String tampungXor="";
  for(int i=0;i<8;i++){
    int hasil=((int)biner1.charAt(i)^(int)biner2.charAt(i));
    tampungXor.concat(hasil);
  }
  return tampungXor;
}
void KirimKunci(String inputKunci){
  bool rslt;
  radio.stopListening();
  char tampungKirim[8];
  for(int i=0;i<8;i++){
    int hasil=inputKunci.charAt(i);
    tampungKirim[i]=hasil;
  }
  rslt=radio.write(&tampungKirim, sizeof(tampungKirim));
  radio.startListening();
  delay(500);
  if(!rslt){
    KirimKunci(inputKunci);
  }
}
String TerimaKunci(){
  char terimaKunci[8];
  if(radio.available()){
    radio.read(&terimaKunci, sizeof(terimaKunci) );
  }
  String tampungTerimaKunci="";
  for(int i=0;i<8;i++){
    tampungTerimaKunci.concat(terimaKunci[i]);
  }
  if(tampungTerimaKunci.length()==8){
    return tampungTerimaKunci;
  }else{
    TerimaKunci();
  }
}
void pengirimTPP(){
  for(int i=0;i<8;i++){
    long randNumber=random(2);
    kunciTPP.concat(randNumber);
  }
  String kunciRandom="";
  for(int i=0;i<8;i++){
    long randNumber=random(2);
    kunciRandom.concat(randNumber);
  }
  String tampungStep1=vernamCipher(kunciTPP,kunciRandom);
  KirimKunci(tampungStep1);
  String tampungStep2=TerimaKunci();
  String tampungStep3=vernamCipher(tampungStep2,kunciRandom);
  KirimKunci(tampungStep3);
  return;
}
void KirimData(String inputEsp01){
  String ciphertext=vernamCipher(inputEsp01,kunciTPP);
  String tampungKunciBaru="";
  for(int i=0;i<8;i++){
    long randNumber=random(2);
    tampungKunciBaru.concat(randNumber);
  }
  String ciphertext_kunci=vernamCipher(tampungKunciBaru,kunciTPP);
  char tampungCiphertext[16];
  for(int i=0;i<16;i++){
    if(i<8){
      tampungCiphertext[i]=ciphertext.charAt(i);
    }else{
      tampungCiphertext[i]=ciphertext_kunci.charAt(i-8);
    }
  }
  bool rslt;
  radio.stopListening();
  rslt=radio.write(&tampungCiphertext, sizeof(tampungCiphertext));
  if(rslt){
    mySerial.print("O");
    kunciTPP=tampungKunciBaru;
  }else{
    mySerial.print("E");
  }
  radio.startListening();
}
void penerimaTPP(){
  if(radio.available()){
    String kunciRandom="";
    for(int i=0;i<8;i++){
      long randNumber=random(2);
      kunciRandom.concat(randNumber);
    }
    String tampungStep1=TerimaKunci();
    String tampungStep2=vernamCipher(tampungStep1,kunciRandom);
    KirimKunci(tampungStep2);
    String tampungStep3=TerimaKunci();
    kunciTPP=vernamCipher(tampungStep3,kunciRandom);
  }
  return;
}
void TerimaData(){
  if(radio.available()){
    char dataReceived[16];
    radio.read(&dataReceived, sizeof(dataReceived) );
    String ciphertext="";
    String kunciBaru="";
    for(int i=0;i<16;i++){
      if(i<8){
        ciphertext.concat(dataReceived[i]);
      }else{
        kunciBaru.concat(dataReceived[i]);
      }
    }
    if(ciphertext.length()==8 && kunciBaru.length()==8){
      String binerPlaintext=vernamCipher(ciphertext,kunciTPP);
      String tampungKunciBaru=vernamCipher(kunciBaru,kunciTPP);
      kunciTPP=tampungKunciBaru;
      tampungKunciBaru="";
      mySerial.print(binerPlaintext);
    }
  }
}