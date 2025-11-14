#include"w25flash.h"
#define MAX_TIMEOUT 200

static inline HAL_StatusTypeDef W25Q_SendByte(uint8_t byte){
  return HAL_SPI_Transmit(&SPI_PORT,&byte,1,MAX_TIMEOUT);
}

static inline HAL_StatusTypeDef W25Q_SendBytes(uint8_t* pData,uint16_t size){
  return HAL_SPI_Transmit(&SPI_PORT,pData,size,MAX_TIMEOUT);
}

uint8_t W25Q_ReceiveByte(){
  uint8_t byte=0;
  HAL_SPI_Receive(&SPI_PORT,&byte,1,MAX_TIMEOUT);
  return byte;
}

HAL_StatusTypeDef W25Q_ReceiveBytes(uint8_t* pData,uint16_t size){
  return HAL_SPI_Receive(&SPI_PORT,pData,size,MAX_TIMEOUT);
}

uint16_t W25Q_ReadID(void){
  uint16_t temp=0;
  __Select_Flash();
  W25Q_SendByte(0x90);
  W25Q_SendByte(0x00);
  W25Q_SendByte(0x00);
  W25Q_SendByte(0x00);
  temp=W25Q_ReceiveByte() << 8;
  temp |= W25Q_ReceiveByte();
  __Deselect_Flash();
  return temp;
}

uint8_t W25Q_ReadSR1(void){
  uint8_t byte=0;
  __Select_Flash();
  W25Q_SendByte(0x05);
  byte=W25Q_ReceiveByte();
  __Deselect_Flash();
  return byte;
}

uint8_t W25Q_ReadSR2(void){
  uint8_t byte=0;
  __Select_Flash();
  W25Q_SendByte(0x35);
  byte=W25Q_ReceiveByte();
  __Deselect_Flash();
  return byte;
}

uint32_t W25Q_Wait_Time(void){
  uint8_t SR1=0;
  uint32_t start=HAL_GetTick();
  do{
    SR1=W25Q_ReadSR1();
  }while((SR1 & 0x01) == 0x01);
  return HAL_GetTick()-start;
}

HAL_StatusTypeDef W25Q_WriteEn(void){
  __Select_Flash();
  HAL_StatusTypeDef status=W25Q_SendByte(0x06);
  __Deselect_Flash();
  W25Q_Wait_Time();
  return status;
}

uint32_t W25Q_Addr_byBlock(uint8_t BlockNo){
  uint32_t addr=BlockNo;
  addr=addr<<16;
  return  addr;
}

uint32_t W25Q_Addr_bySextor(uint16_t SectorNo){
  if(SectorNo > 4095) SectorNo=0;
  uint32_t addr=SectorNo;
  addr=addr<<12;
  return addr;
}

uint32_t W25Q_Addr_byPage(uint16_t PageNo){
  uint32_t addr=PageNo;
  addr=addr<<8;
  return addr;
}

uint32_t W25Q_Addr_byBS(uint8_t BN,uint8_t SubSN){
  if(SubSN > 15) SubSN=0;
  uint32_t addr=BN;
  addr=addr<<16;
  uint32_t offset=SubSN;
  offset=offset<<12;
  return addr+offset;
}

uint32_t W25Q_Addr_byBSP(uint8_t BN,uint8_t SubSN,uint8_t SubPN){
  if(SubSN >15 ) SubSN=0;
  if(SubPN >15 ) SubPN=0;
  uint32_t addr=BN;
  addr=addr<<16;
  uint32_t offset=SubSN;
  offset=offset<<12;
  addr+=offset;
  offset=SubPN;
  offset=offset<<8;
  addr+=offset;
  return addr;
}

void W25Q_SplitAddr(uint32_t Addr,uint8_t *h,uint8_t* m,uint8_t* l){
  *h=(Addr>>16);
  Addr=Addr & 0x0000ffff;
  *m=(Addr>>8);
  *l=Addr & 0x000000ff;
}

void W25Q_EraseSector(uint32_t addr){
  W25Q_WriteEn();
  __Select_Flash();
  uint8_t b2,b3,b4;
  W25Q_SplitAddr(addr,&b2,&b3,&b4);
  W25Q_SendByte(0x20);
  W25Q_SendByte(b2);
  W25Q_SendByte(b3);
  W25Q_SendByte(b4);
  __Deselect_Flash();
  W25Q_Wait_Time();
}

void W25Q_ReadBytes(uint32_t addr,uint8_t*pBuf,uint16_t count){
  uint8_t b2,b3,b4;
  W25Q_SplitAddr(addr,&b2,&b3,&b4);
  __Select_Flash();
  W25Q_SendByte(0x30);
  W25Q_SendByte(b2);
  W25Q_SendByte(b3);
  W25Q_SendByte(b4);
  W25Q_ReceiveBytes(pBuf,count);
  __Deselect_Flash();
}

void W25Q_WriteInPage(uint32_t addr,uint8_t* pBuf,uint16_t count){
  uint8_t b2,b3,b4;
  W25Q_SplitAddr(addr,&b2,&b3,&b4);
  W25Q_WriteEn();
  __Select_Flash();
  W25Q_SendByte(0x02);
  W25Q_SendByte(b2);
  W25Q_SendByte(b3);
  W25Q_SendByte(b4);
  W25Q_SendBytes(pBuf,count);
  __Deselect_Flash();
  W25Q_Wait_Time();
}

void W25Q_WriteSector(uint32_t addr,const uint8_t* pbuf,uint16_t count){
  uint8_t secCount=(count/FLASH_SECTOR_SIZE);
  if((count % FLASH_SECTOR_SIZE) > 0){
    secCount++;
  }
  uint32_t startAddr=addr;
  for(uint8_t k=0;k<secCount;++k){
    W25Q_EraseSector(startAddr);
    startAddr+=FLASH_SECTOR_SIZE;
  }
  uint16_t left=count%FLASH_PAGE_SIZE_F1;
  uint16_t pcCount=count/FLASH_PAGE_SIZE_F1;
  uint8_t* buff=pbuf;
  for(uint16_t i=0;i<pcCount;++i){
    W25Q_WriteInPage(addr,buff,FLASH_PAGE_SIZE_F1);
    addr+=FLASH_PAGE_SIZE_F1;
    buff+=FLASH_PAGE_SIZE_F1;
  }
  if(left > 0) W25Q_WriteInPage(addr,buff,left);
}

void W25Q_WriteDMA(uint32_t addr,uint8_t* PageWrite,uint16_t size){
  uint8_t b2,b3,b4;
  W25Q_SplitAddr(addr,&b2,&b3,&b4);
  W25Q_WriteEn();
  W25Q_Wait_Time();
  __Select_Flash();
  W25Q_SendByte(0x02);
  W25Q_SendByte(b2);
  W25Q_SendByte(b3);
  W25Q_SendByte(b4);
  HAL_SPI_Transmit_DMA(&SPI_PORT,PageWrite,size);
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi){
  __Deselect_Flash();
  W25Q_Wait_Time();
}

void W25Q_ReadDMA(uint32_t addr,uint8_t *PageRead,uint16_t size){
  uint8_t b2,b3,b4;
  W25Q_SplitAddr(addr,&b2,&b3,&b4);
  __Select_Flash();
  W25Q_SendByte(0x03);
  W25Q_SendByte(b2);
  W25Q_SendByte(b3);
  W25Q_SendByte(b4);
  HAL_SPI_Receive_DMA(&SPI_PORT,PageRead,size);
}

void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi){
  __Deselect_Flash();
  W25Q_Wait_Time();
}