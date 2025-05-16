#include <kbd.h>

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
    
    
    
    char* _kbd_readKeyCombo(){
        uint8_t startPress = _kbd_read();
        uint8_t secondPress = _kbd_read();
        if((startPress & secondPress) == 0){
            return scancode_to_ascii[startPress];
        }        
        return 0;
    }