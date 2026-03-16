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

#ifndef NDSCART_CARTRETAILBT_H
#define NDSCART_CARTRETAILBT_H

#include "CartRetail.h"

namespace melonDS::NDSCart
{

// CartRetailBT - Pokémon Typing Adventure (SPI BT controller) (WIP)
enum CartBTState
{
    Waiting,
    Starting,
    CommandLength,
    CommandData,
    EventLength,
    EventData
};

class CartRetailBT : public CartRetail
{
public:
    CartRetailBT(const u8* rom, u32 len, u32 chipid, ROMListEntry romparams, std::unique_ptr<u8[]>&& sram, u32 sramlen, void* userdata);
    CartRetailBT(std::unique_ptr<u8[]>&& rom, u32 len, u32 chipid, ROMListEntry romparams, std::unique_ptr<u8[]>&& sram, u32 sramlen, void* userdata);
    ~CartRetailBT() override;

    u8 SPITransmitReceive(u8 val) override;
    bool IsIRQ() override;

private:
    bool isInterrupt = false;
    bool isDebug = false;

    CartBTState currentState = Waiting;

    u8 BtCmd[2] = {0, 0};
    u16 CmdLength = 0;

    u8 CmdData[257];
    u16 CmdPtr = 0;

    u8 RespData[1023];
    u16 RespPtr = 0;
    u16 RespLen = 0;

    void AppendRespByte(u8 val);
    void AppendCommandComplete(u16 opcode, u8 data[], u32 len);

    void ProcessBTCommand();

    u8 inquiryMode = 0;
};

}

#endif
