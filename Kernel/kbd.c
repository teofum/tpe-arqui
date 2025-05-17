#include <kbd.h>

#define BUFFER_SIZE    64

#define SHIFT 0x2A
#define SHIFT_R 0x36
#define BACKSPACE 0x0E
#define CTRL 0x1D
#define ALT 0x38
#define CAPLOCK 0x3A

    /*
    // Keyboard us, 
    // todo: revisar los valores
    */
    const char scancode_to_ascii[128] = {
     0  , 27 ,'1' ,'2' ,'3' ,'4' ,'5' ,'6' , // 0x00 - 0x07
    '7' ,'8' ,'9' ,'0' ,'-' ,'=' ,'\b','\t', // 0x08 - 0x0F
    'q' ,'w' ,'e' ,'r' ,'t' ,'y' ,'u' ,'i' , // 0x10 - 0x17
    'o' ,'p' ,'[' ,']' ,'\n', 0  ,'a' ,'s' , // 0x18 - 0x1F
    'd' ,'f' ,'g' ,'h' ,'j' ,'k' ,'l' ,';' , // 0x20 - 0x27
    '\'', '`', 0  ,'\\','z' ,'x' ,'c' ,'v' , // 0x28 - 0x2F
    'b' ,'n' ,'m' ,',' ,'.' ,'/' , 0  ,'*' , // 0x30 - 0x37
     0  ,' ' , 0  , 0  , 0  , 0  , 0  , 0  , // 0x38 - 0x3F
     0  , 0  , 0  , 0  , 0  , 0  , 0  , '7', // 0x40 - 0x47
    '8' ,'9' ,'-' ,'4' ,'5' ,'6' ,'+' ,'1' , // 0x48 - 0x4F
    '2' ,'3' ,'0' ,'.' , 0  , 0  , 0  , 0  , // 0x50 - 0x57
     0  , 0  , 0  , 0  , 0  , 0  , 0  , 0    // 0x58 - 0x5F
    };

    const char scancode_to_ascii_shifted[128] = {
     0  , 27 ,'!' ,'@' ,'#' ,'$' ,'%' ,'^' , // 0x00 - 0x07
    '&' , '*','(' ,')' ,'_' ,'+' ,'\b','\t', // 0x08 - 0x0F
    'Q' ,'W' ,'E' ,'R' ,'T' ,'Y' ,'U' ,'I' , // 0x10 - 0x17
    'O' ,'P' ,'{' ,'}' ,'\n', 0  , 'A','S' , // 0x18 - 0x1F
    'D' ,'F' ,'G' ,'H' ,'J' ,'K' ,'L' ,':' , // 0x20 - 0x27
    '"' ,'~' , 0  ,'|' ,'Z' ,'X' ,'C' ,'V' , // 0x28 - 0x2F
    'B' ,'N' ,'M' ,'<' ,'>' ,'?' , 0  ,'*' , // 0x30 - 0x37
     0  ,' ' , 0  , 0  , 0  , 0  , 0  , 0  , // 0x38 - 0x3F
     0  , 0  , 0  , 0  , 0  , 0  , 0  ,'7' , // 0x40 - 0x47
    '8' ,'9' ,'-' ,'4' ,'5' ,'6' ,'+' ,'1' , // 0x48 - 0x4F
    '2' ,'3' ,'0' ,'.' , 0  , 0  , 0  , 0  , // 0x50 - 0x57
     0  , 0  , 0  , 0  , 0  , 0  , 0  , 0    // 0x58 - 0x5F
    };
    
    const char* scancode_to_special[128] = {
    [0x00] = "NULL",
    [0x01] = "ESC",
    [0x0E] = "Backspace",
    [0x0F] = "Tab",
    [0x1C] = "Enter",
    [0x1D] = "Ctrl",
    [0x2A] = "Shift",
    [0x36] = "Right Shift",
    [0x38] = "Alt",
    [0x39] = "Space",
    [0x3A] = "CapsLock",
    [0x3B] = "F1",
    [0x3C] = "F2",
    [0x3D] = "F3",
    [0x3E] = "F4",
    [0x3F] = "F5",
    [0x40] = "F6",
    [0x41] = "F7",
    [0x42] = "F8",
    [0x43] = "F9",
    [0x44] = "F10",
    [0x57] = "F11",
    [0x58] = "F12",
    [0x45] = "NumLock",
    [0x46] = "ScrollLock"
    };
    
    
    // /* Buffer circular con flags adicionales*/
    // struct buffer{
    // char* data[BUFFER_SIZE];
    // int write_pos;
    // uint8_t isChar; //flag
    // };

    /* Verifica si el buffer está lleno */
    uint8_t isBuffFull(struct buffer buff) {
      return buff.write_pos<BUFFER_SIZE;
    }

    /* Agrega un carácter al buffer */
    void addCharToBuff(char* c,struct buffer buff) {
      buff.data[buff.write_pos]=c;
      buff.write_pos+=1;
    }

    struct buffer _kbd_readKeyCombo(){

      struct buffer buff = {.data={0}, .write_pos=0, .isChar=1};

      char firstKey =_kbd_read();
      addCharToBuff(scancode_to_ascii[firstKey],buff);

      for(int i=0;i<BUFFER_SIZE;i++){
        char currKey =_kbd_read();

        if( currKey & 0x80 && currKey & firstKey){ //deberia validar si es el release de la primera tecla
          return buff;// esto no se si se libera ;- ;
        }else if(scancode_to_special[currKey]!=0){//si hay teclas
          buff.isChar=0;
          addCharToBuff(scancode_to_special[currKey],buff);
        }else{
          addCharToBuff(scancode_to_special[currKey],buff);
        }
      
      }
      return buff;

    }

