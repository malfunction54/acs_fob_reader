/*
 *  RFID keyfob reader for https://theLab.ms
 *  Shamelessly hacked together from bits and pieces found elsewhere 
 *  by @lego - malfunction54@therac25.net
 *  
 *  I've attributed the code to the original (purportedly) authors
 *  - base tag code (c) Michael Schoeffler 2018, http://www.mschoeffler.de
 *    code: https://mschoeffler.com/2018/01/05/arduino-tutorial-how-to-use-the-rdm630-rdm6300-rfid-reader/
 *  - LCD code by Saeed Hosseini @ Electropeak https://electropeak.com/learn/
 */


/*
Arduino 2x16 LCD - Detect Buttons
modified on 18 Feb 2019
by Saeed Hosseini @ Electropeak
https://electropeak.com/learn/
*/
#include <LiquidCrystal.h>
//LCD pin to Arduino
const int pin_RS = 8; 
const int pin_EN = 9; 
const int pin_d4 = 4; 
const int pin_d5 = 5; 
const int pin_d6 = 6; 
const int pin_d7 = 7; 
const int pin_BL = 10; 
LiquidCrystal lcd( pin_RS,  pin_EN,  pin_d4,  pin_d5,  pin_d6,  pin_d7);

// (c) Michael Schoeffler 2018, http://www.mschoeffler.de
#include <SoftwareSerial.h>
const int BUFFER_SIZE = 14; // RFID DATA FRAME FORMAT: 1byte head (value: 2), 10byte data (2byte version + 8byte tag), 2byte checksum, 1byte tail (value: 3)
const int DATA_SIZE = 10; // 10byte data (2byte version + 8byte tag)
const int DATA_VERSION_SIZE = 2; // 2byte version (actual meaning of these two bytes may vary)
const int DATA_TAG_SIZE = 8; // 8byte tag
const int CHECKSUM_SIZE = 2; // 2byte checksum

//SoftwareSerial ssrfid = SoftwareSerial(6,8); 
SoftwareSerial ssrfid = SoftwareSerial(11,12); 

uint8_t buffer[BUFFER_SIZE]; // used to store an incoming data frame 
int buffer_index = 0;

void setup() {
  Serial.begin(9600); 
  ssrfid.begin(9600);
  ssrfid.listen(); 
  
  lcd.begin(16, 2);
  lcd.setCursor(0,0);
  Serial.println("Ready to scan a key fob");
  lcd.print("Ready to scan");
  lcd.setCursor(0,1);
  lcd.print("a key fob");
}

void loop() {
  if (ssrfid.available() > 0){
    bool call_extract_tag = false;
    
    int ssvalue = ssrfid.read(); // read 
    if (ssvalue == -1) { // no data was read
      return;
    }

    if (ssvalue == 2) { // RDM630/RDM6300 found a tag => tag incoming 
      buffer_index = 0;
    } else if (ssvalue == 3) { // tag has been fully transmitted       
      call_extract_tag = true; // extract tag at the end of the function call
    }

    if (buffer_index >= BUFFER_SIZE) { // checking for a buffer overflow (It's very unlikely that an buffer overflow comes up!)
      //Serial.println("Error: Buffer overflow detected!");
      lcd.print("Error: PANIK!");
      return;
    }
    
    buffer[buffer_index++] = ssvalue; // everything is alright => copy current value to buffer

    if (call_extract_tag == true) {
      if (buffer_index == BUFFER_SIZE) {
        unsigned tag = extract_tag();
      } else { // something is wrong... start again looking for preamble (value: 2)
        buffer_index = 0;
        return;
      }
    }
  }    
}

/* 
 *  theLab's tag reader assumes Weigand encoding,
 *  as described here: https://www.networkingsmith.com/learning-about-rfid-and-wiegand/
 *  
 *  This is why the IDs in the ACS don't match the numbers printed on the fobs/cards.
 *  The fobs/cards use mifare encoding as described in the Schoeffler article.
 *  
 *  as a result, extract_tag() is modified from the original to produce the expected ID
 *  
 */
unsigned extract_tag() {
    uint8_t msg_head = buffer[0];
    uint8_t *msg_data = buffer + 1; // 10 byte => data contains 2byte version + 8byte tag
    uint8_t *msg_data_version = msg_data;
    uint8_t *msg_data_tag = msg_data + 2;
    uint8_t *msg_checksum = buffer + 11; // 2 byte
    uint8_t msg_tail = buffer[13];
    long    data_ver;
    long    data_tag;
    long    chksum;

    // print message that was sent from RDM630/RDM6300
    Serial.println("--------");
    Serial.println("Complete buffer contents: ");
    Serial.println("Message-Data (Binary): ");
    for (int i = 0; i < BUFFER_SIZE; ++i) {
      //Serial.print(char(buffer[i]));
      printBinary(buffer[i]);
      //Serial.print(" ");
    }
    Serial.println();
    // full data is 14 bytes, we just need the first (most significant)
    //   26 bits (3 Bytes and 2 bits)
    // so grab the most significant 4 bytes
    // shift right 6 bits
    // shift right 1 bit to drop ending parity bit
    // mask out most significant bit to ignore beginning parity bit
    // Do we have the wiegand number?

    Serial.print("Message-Head: ");
    Serial.println(msg_head);
    Serial.print("Message-Head (bin): ");
    printBinary(msg_head);
    Serial.println();

    Serial.println("Message-Data (HEX): ");
    for (int i = 0; i < DATA_VERSION_SIZE; ++i) {
      Serial.print(char(msg_data_version[i]));
    }
    Serial.println(" (version)");

    Serial.println("Message-Data (bin): ");
    data_ver = hexstr_to_value(msg_data_version, DATA_VERSION_SIZE);
    printBinary(data_ver);
    Serial.println(" (version)");

    for (int i = 0; i < DATA_TAG_SIZE; ++i) {
      Serial.print(char(msg_data_tag[i]));
    }
    Serial.println(" (tag)");

    Serial.print("Message-Checksum (HEX): ");
    for (int i = 0; i < CHECKSUM_SIZE; ++i) {
      Serial.print(char(msg_checksum[i]));
    }
    Serial.println("");

    Serial.print("Message-Tail: ");
    Serial.println(msg_tail);

    Serial.print("Message-Tail (bin): ");
    printBinary(msg_tail);
    Serial.println();

    Serial.println("--");

    long tag = hexstr_to_value(msg_data_tag, DATA_TAG_SIZE);
    Serial.print("Extracted Tag: ");
    Serial.println(tag);
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print(tag);
          
    long checksum = 0;
    for (int i = 0; i < DATA_SIZE; i+= CHECKSUM_SIZE) {
      long val = hexstr_to_value(msg_data + i, CHECKSUM_SIZE);
      checksum ^= val;
    }
    Serial.print("Extracted Checksum (HEX): ");
    Serial.print(checksum, HEX);
    if (checksum == hexstr_to_value(msg_checksum, CHECKSUM_SIZE)) { // compare calculated checksum to retrieved checksum
      Serial.print(" (OK)"); // calculated checksum corresponds to transmitted checksum!
    } else {
      Serial.print(" (NOT OK)"); // checksums do not match
    }

    Serial.println("");
    Serial.println("--------");

    return tag;
}

long hexstr_to_value(char *str, unsigned int length) {
  // converts a hexadecimal value (encoded as ASCII string) to a numeric value
  char* copy = malloc((sizeof(char) * length) + 1); 
  memcpy(copy, str, sizeof(char) * length);
  copy[length] = '\0'; 
  // the variable "copy" is a copy of the parameter "str". "copy" has an additional '\0' element to make sure that "str" is null-terminated.
  long value = strtol(copy, NULL, 16);  // strtol converts a null-terminated string to a long value
  free(copy); // clean up 
  return value;
}

void printBinary(byte inByte)
{
  for (int b = 7; b >= 0; b--)
  {
    Serial.print(bitRead(inByte, b));
  }
}
