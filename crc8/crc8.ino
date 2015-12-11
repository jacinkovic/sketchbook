const int Mirf_payload = 16;
byte mirf_data[Mirf_payload]; 

int i;
byte mirf_data_checksum;


//kontrolny sucet
byte crc = 0x00;
//mirf_data_checksum = 0; 
for (i = 0; i < Mirf.payload-1; i++) {
  //mirf_data_checksum = mirf_data_checksum + mirf_data[i];
  byte extract = mirf_data[i];
  for (byte tempI = 8; tempI; tempI--) {
    byte sum = (crc ^ extract) & 0x01;
    crc >>= 1;
    if (sum) {
      crc ^= 0x8C;
    }
    extract >>= 1;
  }
}
mirf_data_checksum = crc;
mirf_data[Mirf.payload-1] = mirf_data_checksum;



//CRC-8 - based on the CRC8 formulas by Dallas/Maxim
//code released under the therms of the GNU GPL 3.0 license
byte CRC8(const byte *data, byte len) {
  byte crc = 0x00;
  while (len--) {
    byte extract = *data++;
    for (byte tempI = 8; tempI; tempI--) {
      byte sum = (crc ^ extract) & 0x01;
      crc >>= 1;
      if (sum) {
        crc ^= 0x8C;
      }
      extract >>= 1;
    }
  }
  return crc;
}

