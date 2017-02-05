#ifndef H_GATES
#define H_GATES

#include <fstream>
#include <string>
#include "blueprintlib/blueprint.h"
#include "blueprintlib/blocks.h"

enum SWITCH_TYPE {SWITCH_OFF = 0, SWITCH_ON = 1};

class TimerPair
{
    private:
        void SetCoords(uint64_t x, uint64_t y, uint64_t z, TimerBlock* timer)
        {
            timer->Coords.x = x;
            timer->Coords.y = y;
            timer->Coords.z = z;
        }

        void AddHook(SWITCH_TYPE switchType, TimerPair* toSwitch, TimerBlock* switchingTimer)
        {
            switchingTimer->toolbar.AddEntry(switchingTimer->toolbar.FirstEmptySlot(), switchType ? "OnOff_Off" : "OnOff_On", &toSwitch->timerLow);
            switchingTimer->toolbar.AddEntry(switchingTimer->toolbar.FirstEmptySlot(), switchType ? "OnOff_On" : "OnOff_Off", &toSwitch->timerHigh);
        }

        void AddUpdate(TimerPair* toUpdate, TimerBlock* updatingTimer)
        {
            updatingTimer->toolbar.AddEntry(updatingTimer->toolbar.FirstEmptySlot(), "TriggerNow", toUpdate->timerLow);
            updatingTimer->toolbar.AddEntry(updatingTimer->toolbar.FirstEmptySlot(), "TriggerNow", toUpdate->timerHigh);
        }
    public:
        TimerBlock timerLow;
        TimerBlock timerHigh;
    public:
        TimerPair()
        {
            timerLow.Enabled = true;
            timerHigh.Enabled = false;
            timerLow.CustomName = "Timer L";
            timerHigh.CustomName = "Timer H";
        }

        void SetCoordsLow(uint64_t x, uint64_t y, uint64_t z)
        {
            SetCoords(x, y, z, &timerLow);
        }
        void SetCoordsHigh(uint64_t x, uint64_t y, uint64_t z)
        {
            SetCoords(x, y, z, &timerHigh);
        }

        void AppendBlocks(CubeGrid* cubegrid)
        {
            cubegrid->blocks.AddBlock(&timerLow);
            cubegrid->blocks.AddBlock(&timerHigh);
        }

        void AddHookLow(SWITCH_TYPE switchType, TimerPair* toSwitch)
        {
            AddHook(switchType, toSwitch, &timerLow);
        }
        void AddHookHigh(SWITCH_TYPE switchType, TimerPair* toSwitch)
        {
            AddHook(switchType, toSwitch, &timerHigh);
        }
        void AddHookBoth(TimerPair* toSwitch)
        {
            AddHookLow(SWITCH_OFF, toSwitch);
            AddHookHigh(SWITCH_ON, toSwitch);
        }

        void AddUpdateLow(TimerPair* toUpdate)
        {
            AddUpdate(toUpdate, &timerLow);
        }
        void AddUpdateHigh(TimerPair* toUpdate)
        {
            AddUpdate(toUpdate, &timerHigh);
        }
        void AddUpdateBoth(TimerPair* toUpdate)
        {
            AddUpdateLow(toUpdate);
            AddUpdateHigh(toUpdate);
        }

        void AddHookUpdate(TimerPair* toSU)
        {
            AddHookBoth(toSU);
            AddUpdateBoth(toSU);
        }
};

template <unsigned input_count> class LogicGate
{
    protected:


        std::string GenerateLetter(unsigned index)
        {
            const char startLetter = 'A';
            const char lastLetter = 'Z';
            char difference = lastLetter - startLetter + 1;
            if ((index / difference) == 0)
                return std::string(1, static_cast<char>(index+startLetter));
            else
            {
                std::string ret;
                ret.append(GenerateLetter((index/difference) - 1));
                ret.append(std::string(1, static_cast<char>((index % difference)+startLetter)));
                return ret;
            }
        }
};

template <unsigned input_count> class AndGate : public LogicGate<input_count>
{
    friend class Circuit;
    private:
        CubeGrid cubegrid;
        TimerPair inputs[input_count];
        TimerPair output;
        TimerBlock updater;
    public:
        AndGate()
        {
            for (int i = 0; i < input_count; ++i)
            {
                inputs[i].SetCoordsLow(i, 0, 0);
                inputs[i].SetCoordsHigh(i, 1, 0);
                inputs[i].timerLow.CustomName = std::string("AND input ") + this->GenerateLetter(i) + std::string(" L");
                inputs[i].timerHigh.CustomName = std::string("AND input ") + this->GenerateLetter(i) + std::string(" H");
                inputs[i].AddHookUpdate(&output);
            }
            output.SetCoordsLow(input_count, 0, 0);
            output.SetCoordsHigh(input_count, 1, 0);
            output.timerLow.CustomName = "AND output L";
            output.timerHigh.CustomName = "AND output H";
            updater.Coords.x = input_count + 1;
            updater.CustomName = "AND updater";

            for (int i = 0; i < input_count; ++i)
                updater.toolbar.AddEntry(updater.toolbar.FirstEmptySlot(), "TriggerNow", &inputs[i].timerHigh);
            for (int i = 0; i < input_count; ++i)
                updater.toolbar.AddEntry(updater.toolbar.FirstEmptySlot(), "TriggerNow", &inputs[i].timerLow);
        }

        void AppendBlocks()
        {
            for (int i = 0; i < input_count; ++i)
            {
                inputs[i].AppendBlocks(&cubegrid);
            }
            output.AppendBlocks(&cubegrid);
            cubegrid.blocks.AddBlock(&updater);
        }

        void HookOutputTo(TimerPair* input, TimerBlock* updater)
        {
            output.AddHookBoth(input);
            output.timerLow.toolbar.AddEntry(output.timerLow.toolbar.FirstEmptySlot(), "TriggerNow", updater);
            output.timerHigh.toolbar.AddEntry(output.timerHigh.toolbar.FirstEmptySlot(), "TriggerNow", updater);
        }

        void AppendToName(std::string toAppend)
        {
            for (int i = 0; i < input_count; ++i)
            {
                inputs[i].timerLow.CustomName() += toAppend;
                inputs[i].timerHigh.CustomName() += toAppend;
            }
            output.timerLow.CustomName() += toAppend;
            output.timerHigh.CustomName() += toAppend;
            updater.CustomName() += toAppend;
        }
};

class Circuit
{
    private:
        Blueprint blueprint;
        //CubeGrid cubegrid;
    public:
        AndGate<3> andGate1;
        AndGate<3> andGate2;
        AndGate<2> andGate3;

        void BuildXml()
        {
            andGate1.AppendToName(" 1");
            andGate2.AppendToName(" 2");
            andGate3.AppendToName(" 3");
            andGate1.HookOutputTo(&andGate3.inputs[0], &andGate3.updater);
            andGate2.HookOutputTo(&andGate3.inputs[1], &andGate3.updater);
            andGate1.AppendBlocks();
            andGate2.AppendBlocks();
            andGate3.AppendBlocks();
            andGate1.cubegrid.TranslateCoords(0, 0, 1);
            andGate2.cubegrid.TranslateCoords(0, 0, 3);
            andGate3.cubegrid.TranslateCoords(0, 0, 5);
            ArmorBlock armor;
            armor.Coords.z = 2;
            andGate1.cubegrid.blocks.AddBlock(armor);
            armor.Coords.z = 4;
            andGate2.cubegrid.blocks.AddBlock(armor);
            blueprint.Cubegrids.push_back(andGate1.cubegrid);
            blueprint.Cubegrids.push_back(andGate2.cubegrid);
            blueprint.Cubegrids.push_back(andGate3.cubegrid);

            std::fstream output("bp.sbc", std::fstream::out);
            if (output.is_open())
                blueprint.Print(output, false);
            else std::cout<<"Error writing to file"<<std::endl;
        }
};

#endif // H_GATES
