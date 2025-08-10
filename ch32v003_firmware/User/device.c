#include "device.h"
#include "ch32v00x_flash.h"


uint8_t device_uid[4];
uint8_t DEVICE_ADDR = 0x00;

uint32_t GetMCUUID1( void ) { return( *( uint32_t * )0x1FFFF7E8 ); }
uint32_t GetMCUUID2( void ) { return( *( uint32_t * )0x1FFFF7EC ); }

void init_device(void) {
    uint32_t uid1 = GetMCUUID1();
    uint32_t uid2 = GetMCUUID2();

    device_uid[0] = uid1 >> 24;
    device_uid[1] = uid1 >> 16;
    device_uid[2] = uid2 >> 24;
    device_uid[3] = uid2 >> 16;

    // ���ѧԧ��اѧ֧� �����ѧߧקߧߧ�� �ѧէ�֧� �ڧ� flash (�֧�ݧ� �֧���)
    DEVICE_ADDR = load_device_address();
}

void save_device_address(uint8_t addr) {
    FLASH_Unlock();

    // ����֧�֧�� ��էߧ� ����ѧߧڧ��
    FLASH_ErasePage(FLASH_ADDR_DEVICE);

    FLASH_ProgramHalfWord(FLASH_ADDR_DEVICE, addr);

    FLASH_Lock();
}

uint8_t load_device_address() {
    uint8_t value = *(volatile uint8_t *)FLASH_ADDR_DEVICE;
    return (value == 0xFF || value == 0x00) ? 0x00 : value;  // �֧�ݧ� ������ �� �ӧ֧�ߧ��� 0
}