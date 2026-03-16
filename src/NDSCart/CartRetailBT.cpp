/*
    Copyright 2016-2026 melonDS team

    This file is part of melonDS.

    melonDS is free software: you can redistribute it and/or modify it under
    the terms of the GNU General Public License as published by the Free
    Software Foundation, either version 3 of the License, or (at your option)
    any later version.

    melonDS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with melonDS. If not, see http://www.gnu.org/licenses/.
*/

#include "CartRetailBT.h"
#include "../NDS.h"
#include "../Utils.h"

// CartRetailBT: NDS cartridge with Bluetooth transceiver (ie. Pokémon Typing Adventure)
// the BT transceiver is connected to the SPI interface, and replaces regular SRAM

namespace melonDS
{
using Platform::Log;
using Platform::LogLevel;

namespace NDSCart
{

CartRetailBT::CartRetailBT(const u8* rom, u32 len, u32 chipid, ROMListEntry romparams, std::unique_ptr<u8[]>&& sram, u32 sramlen, void* userdata) :
    CartRetailBT(CopyToUnique(rom, len), len, chipid, romparams, std::move(sram), sramlen, userdata)
{
}

CartRetailBT::CartRetailBT(std::unique_ptr<u8[]>&& rom, u32 len, u32 chipid, ROMListEntry romparams, std::unique_ptr<u8[]>&& sram, u32 sramlen, void* userdata) :
    CartRetail(std::move(rom), len, chipid, false, romparams, std::move(sram), sramlen, userdata, CartType::RetailBT)
{
    Log(LogLevel::Info,"POKETYPE CART\n");
}

CartRetailBT::~CartRetailBT() = default;

u8 CartRetailBT::SPITransmitReceive(u8 val)
{
    // NOTE: All info regarding this cart (so far) is derived from the following links:
    // https://gist.github.com/windwakr/812809405341775c3d9e42228f5b8696
    // https://github.com/kynex7510/PTSM/blob/main/PROTOCOL.md
    // The official Bluetooth v2.1 + EDR specification (which the Bluetooth controller in this cart uses;
    // can be found on official Bluetooth website)

    // TODO: Finish (WIP) implementation

    //Log(LogLevel::Debug,"POKETYPE SPI: %02X %d %d - %08X\n", val, pos, last, NDS::GetPC(0));

    // Log(LogLevel::Info,"POKETYPE SPI: %02X\n", val);

    switch (currentState)
    {
    case Waiting:
    {
        if (val == 0xFF)
        {
            // Log(LogLevel::Info, "Initializing command...\n");
            currentState = Starting;
            isInterrupt = true;
        }

        return 0;
    }
    break;
    case Starting:
    {
        if (isBtMsb)
        {
            BtCmd |= val;

            if (BtCmd == 0x0100)
            {
                currentState = CommandLength;
            }
            else if (BtCmd == 0x0200)
            {
                currentState = EventLength;
            }

            isBtMsb = false;
        }
        else
        {
            BtCmd = (val << 8);
            isBtMsb = true;
        }

        return 0;
    }
    break;
    case CommandLength:
    {
        if (!isBtMsb)
        {
            CmdLength = (val << 8);
            isBtMsb = true;
        }
        else
        {
            CmdLength |= val;
            CmdPtr = 0;
            currentState = CommandData;
            isBtMsb = false;
        }

        return 0;
    }
    break;
    case CommandData:
    {
        CmdData[CmdPtr++] = val;

        if (CmdPtr == CmdLength)
        {
            RespLen = 0;

            if (CmdData[0] == 0x01)
            {
                ProcessBTCommand();
                isInterrupt = true;
            }
            else
            {
                Log(LogLevel::Info, "Unrecognized Bluetooth packet indicator of 0x%02x\n", CmdData[0]);
            }

            currentState = Starting;
        }

        return 0;
    }
    break;
    case EventLength:
    {
        if (!isBtMsb)
        {
            isBtMsb = true;
            return (RespLen >> 8);
        }
        else
        {
            isBtMsb = false;
            RespPtr = 0;
            currentState = (RespLen == 0) ? Waiting : EventData;
            return (RespLen & 0xFF);
        }

        return 0;
    }
    break;
    case EventData:
    {
        u8 data = RespData[RespPtr++];

        if (RespPtr >= RespLen)
        {
            RespPtr = 0;
            RespLen = 0;
            currentState = Waiting;
        }

        // Log(LogLevel::Info, "Data is 0x%02x\n", data);

        return data;
    }
    break;
    }

    return 0;
}

void CartRetailBT::AppendRespByte(u8 val)
{
    if (RespLen >= 1023)
    {
        Log(LogLevel::Warn, "Response data overflow\n");
    }

    RespData[RespLen++] = val;
}

void CartRetailBT::AppendCommandComplete(u16 opcode, u8 data[], u32 len)
{
    if (len > 252)
    {
        return;
    }

    u8 dataLen = (u8)len;

    AppendRespByte(0x04);
    AppendRespByte(0x0E);
    AppendRespByte(dataLen + 3);
    AppendRespByte(0x01);
    AppendRespByte((opcode & 0xFF));
    AppendRespByte((opcode >> 8));

    for (u8 i = 0; i < dataLen; i++)
    {
        AppendRespByte(data[i]);
    }
}

void CartRetailBT::ProcessBTCommand()
{
    u16 opcode = ((CmdData[2] << 8) | CmdData[1]);

    u8 ogf = ((opcode >> 10) & 0x3F);
    u16 ocf = (opcode & 0x3FF);

    u8 paramLen = CmdData[3];

    // Log(LogLevel::Info, "Opcode is (ogf = 0x%02x, ocf = 0x%04x)\n", ogf, ocf);

    switch (opcode)
    {
    case 0x0401:
    {
        // TODO: Figure out the appropriate response packet for this command
        Log(LogLevel::Info, "HCI_Inquiry (currently unimplemented)\n");
    }
    break;
    case 0x0C01:
    {
        Log(LogLevel::Info, "HCI_Set_Event_Mask\n");
        u8 resp[1] = {0x00};
        AppendCommandComplete(opcode, resp, 1);
    }
    break;
    case 0x0C03:
    {
        Log(LogLevel::Info, "HCI_Reset\n");
        u8 resp[1] = {0x00};
        AppendCommandComplete(opcode, resp, 1);
    }
    break;
    case 0x0C05:
    {
        Log(LogLevel::Info, "HCI_Set_Event_Filter\n");
        u8 resp[1] = {0x00};
        AppendCommandComplete(opcode, resp, 1);
    }
    break;
    case 0x0C13:
    {
        Log(LogLevel::Info, "HCI_Write_Local_Name\n");
        u8 resp[1] = {0x00};
        AppendCommandComplete(opcode, resp, 1);
    }
    break;
    case 0x0C18:
    {
        Log(LogLevel::Info, "HCI_Write_Page_Timeout\n");
        u8 resp[1] = {0x00};
        AppendCommandComplete(opcode, resp, 1);
    }
    break;
    case 0x0C24:
    {
        Log(LogLevel::Info, "HCI_Write_Class_of_Device\n");
        u8 resp[1] = {0x00};
        AppendCommandComplete(opcode, resp, 1);
    }
    break;
    case 0x0C33:
    {
        Log(LogLevel::Info, "HCI_Host_Buffer_Size\n");
        u8 resp[1] = {0x00};
        AppendCommandComplete(opcode, resp, 1);
    }
    break;
    case 0x0C43:
    {
        Log(LogLevel::Info, "HCI_Write_Inquiry_Scan_Type\n");
        u8 resp[1] = {0x00};
        AppendCommandComplete(opcode, resp, 1);
    }
    break;
    case 0x0C45:
    {
        Log(LogLevel::Info, "HCI_Write_Inquiry_Mode\n");
        u8 data = CmdData[4];
        Log(LogLevel::Info, "Setting inquiry mode to 0x%02x\n", data);
        inquiryMode = data;
        u8 resp[1] = {0x00};
        AppendCommandComplete(opcode, resp, 1);
    }
    break;
    case 0x0C47:
    {
        Log(LogLevel::Info, "HCI_Write_Page_Scan_Type\n");
        u8 resp[1] = {0x00};
        AppendCommandComplete(opcode, resp, 1);
    }
    break;
    case 0x0C49:
    {
        Log(LogLevel::Info, "HCI_Write_AFH_Channel_Assessment_Mode\n");
        u8 resp[1] = {0x00};
        AppendCommandComplete(opcode, resp, 1);
    }
    break;
    case 0x0C52:
    {
        Log(LogLevel::Info, "HCI_Write_Extended_Inquiry_Response\n");
        u8 resp[1] = {0x00};
        AppendCommandComplete(opcode, resp, 1);
    }
    break;
    case 0x0C56:
    {
        Log(LogLevel::Info, "HCI_Write_Simple_Pairing_Mode\n");
        u8 resp[1] = {0x00};
        AppendCommandComplete(opcode, resp, 1);
    }
    break;
    case 0x1001:
    {
        Log(LogLevel::Info, "HCI_Read_Local_Version_Information\n");
        u8 resp[9] = {0x00, 0x04, 0x17, 0x02, 0x04, 0x0F, 0x00, 0x20, 0x41};
        AppendCommandComplete(opcode, resp, 9);
    }
    break;
    case 0x1003:
    {
        Log(LogLevel::Info, "HCI_Read_Local_Supported_Features\n");
        u8 resp[9] = {0x00, 0xFF, 0xFF, 0x8F, 0xFE, 0x9B, 0xFF, 0x79, 0x83};
        AppendCommandComplete(opcode, resp, 9);
    }
    break;
    case 0x1005:
    {
        Log(LogLevel::Info, "HCI_Read_Buffer_Size\n");
        u8 resp[8] = {0x00, 0xFD, 0x03, 0x40, 0x08, 0x00, 0x01, 0x00};
        AppendCommandComplete(opcode, resp, 8);
    }
    break;
    case 0x1009:
    {
        Log(LogLevel::Info, "HCI_Read_BD_ADDR\n");
        u8 resp[7] = {0x00, 0x0D, 0x48, 0xB5, 0xA3, 0xBD, 0x58};
        AppendCommandComplete(opcode, resp, 7);
    }
    break;
    case 0xFC4C:
    case 0xFCEC:
    {
        Log(LogLevel::Info, "HCI_Write_Flash\n");
        u8 resp[1] = {0x00};
        AppendCommandComplete(opcode, resp, 1);
    }
    break;
    case 0xFC4D:
    case 0xFCED:
    {
        Log(LogLevel::Info, "HCI_Read_Flash\n");

        u32 addr = ((CmdData[7] << 24) | (CmdData[6] << 16) | (CmdData[5] << 8) | CmdData[4]);
        u8 size = CmdData[8];

        Log(LogLevel::Info, "Reading %d bytes from flash RAM address of 0x%08x\n", size, addr);

        u8 resp[size];

        AppendCommandComplete(opcode, resp, size);
    }
    break;
    case 0xFC6E:
    {
        Log(LogLevel::Info, "HCI_Read_Controller_Features\n");
        u8 resp[9] = {0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        AppendCommandComplete(opcode, resp, 9);
    }
    break;
    case 0xFC79:
    {
        Log(LogLevel::Info, "HCI_Read_Verbose_Config\n");
        u8 resp[7] = {0x00, 0x13, 0x11, 0x15, 0x02, 0x17, 0x02};
        AppendCommandComplete(opcode, resp, 7);
    }
    break;
    case 0xFF5E:
    case 0xFCEE:
    {
        Log(LogLevel::Info, "HCI_Sector_Erase\n");
        u8 resp[1] = {0x00};
        AppendCommandComplete(opcode, resp, 1);
    }
    break;
    default:
    {
        Log(LogLevel::Info, "Unrecognized Bluetooth opcode of 0x%04x\n", opcode);
    }
    break;
    }
}

bool CartRetailBT::IsIRQ()
{
    if (isInterrupt)
    {
        isInterrupt = false;
        return true;
    }

    return false;
}



}

}
