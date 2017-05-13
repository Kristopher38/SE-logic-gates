#ifndef H_GATES
#define H_GATES

#include <fstream>
#include <string>
#include <bitset>
#include <cmath>
#include "blueprintlib/blueprint.h"
#include "blueprintlib/blocks.h"

class Circuit;
class CircuitCubegridManager;

class TimerPair
{
    friend class DebugInput;
    friend class CircuitCubegridManager;

    protected:
        TimerBlock timerLow;
        TimerBlock timerHigh;

    public:
        enum TIMER {LOW = 0, HIGH = 1};
        TimerPair()
        {
            timerLow.Enabled = true;
            timerHigh.Enabled = false;
            timerLow.CustomName = "L";
            timerHigh.CustomName = "H";
        }
        void Negate()
        {
            timerLow.Enabled = !timerLow.Enabled();
            timerHigh.Enabled = !timerHigh.Enabled();
        }
        void SetCoords(uint64_t x, uint64_t y, uint64_t z, TIMER timerType)
        {
            TimerBlock* timer = timerType ? &timerHigh : &timerLow;
            timer->Coords.x = x;
            timer->Coords.y = y;
            timer->Coords.z = z;
        }
        void AppendToName(std::string toAppend)
        {
            timerLow.CustomName() += toAppend;
            timerHigh.CustomName() += toAppend;
        }
        void PrependToName(std::string toPrepend)
        {
            timerLow.CustomName().insert(0, toPrepend);
            timerHigh.CustomName().insert(0, toPrepend);
        }
        EntityId GetHookLow()
        {
            return this->timerLow.GetEntityId();
        }
        EntityId GetHookHigh()
        {
            return this->timerHigh.GetEntityId();
        }
        void AddSwitch(TimerPair& toSwitch, bool negate = false)
        {
            timerLow.toolbar.AddEntry(negate ? "OnOff_Off" : "OnOff_On", toSwitch.timerLow);
            timerLow.toolbar.AddEntry(negate ? "OnOff_On" : "OnOff_Off", toSwitch.timerHigh);
            timerHigh.toolbar.AddEntry(negate ? "OnOff_On" : "OnOff_Off", toSwitch.timerLow);
            timerHigh.toolbar.AddEntry(negate ? "OnOff_Off" : "OnOff_On", toSwitch.timerHigh);
        }
        void AddUpdate(TimerPair& toUpdate)
        {
            timerLow.toolbar.AddEntry("TriggerNow", toUpdate.timerLow);
            timerLow.toolbar.AddEntry("TriggerNow", toUpdate.timerHigh);
            timerHigh.toolbar.AddEntry("TriggerNow", toUpdate.timerLow);
            timerHigh.toolbar.AddEntry("TriggerNow", toUpdate.timerHigh);
        }
        void AddUpdate(TimerBlock& toUpdate)
        {
            timerLow.toolbar.AddEntry("TriggerNow", toUpdate);
            timerHigh.toolbar.AddEntry("TriggerNow", toUpdate);
        }
        void Connect(TimerPair& toConnect)
        {
            AddSwitch(toConnect, false);
            AddUpdate(toConnect);
        }
        void NegatedConnect(TimerPair& toConnect)
        {
            AddSwitch(toConnect, true);
            AddUpdate(toConnect);
        }
};

typedef TimerBlock Updater;
/*class Updater : public TimerBlock {};*/

struct Hook
{
    TimerPair& input;
    Updater& updater;
    Hook(TimerPair& _input, Updater& _updater) : input(_input), updater(_updater) {}
};

template <unsigned input_count> class LogicGate
{
    friend class DebugInput;
    friend class CircuitCubegridManager;

    protected:
        TimerPair inputs[input_count];
        TimerPair output;
        Updater updater;

        LogicGate()
        {
            SetupInputs();
            SetupOutput();
        }

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

        virtual void SetupOutput()
        {
            output.SetCoords(input_count, 0, 0, TimerPair::LOW);
            output.SetCoords(input_count, 1, 0, TimerPair::HIGH);
            output.PrependToName("output ");
        }

        virtual void SetupInputs()
        {
            for (int i = 0; i < input_count; ++i)
            {
                inputs[i].SetCoords(i, 0, 0, TimerPair::LOW);
                inputs[i].SetCoords(i, 1, 0, TimerPair::HIGH);
                inputs[i].PrependToName(std::string("input ") + this->GenerateLetter(i) + std::string(" "));
            }
        }

        virtual void SetupUpdater()
        {
            updater.CustomName().insert(0, "updater ");
        }

    public:
        virtual void AppendToName(std::string toAppend)
        {
            for (int i = 0; i < input_count; ++i)
                inputs[i].AppendToName(toAppend);
            output.AppendToName(toAppend);
        }
        virtual void HookOutputTo(Hook hook)
        {
            output.AddSwitch(hook.input);
            output.AddUpdate(hook.updater);
        }
        virtual Hook GetHook(unsigned inputIndex)
        {
            if (inputIndex >= input_count)
                throw std::out_of_range("Input index out of range");
            else return Hook(this->inputs[inputIndex], this->updater);
        }
};

template <unsigned input_count> class AndGate : public LogicGate<input_count>
{
    friend class DebugInput;
    friend class CircuitCubegridManager;

    protected:

        void SetupUpdater()
        {
            this->updater.CustomName = "AND updater";
            for (unsigned i = 0; i < input_count; ++i)
                this->updater.toolbar.AddEntry("TriggerNow", this->inputs[i].GetHookHigh());
            for (unsigned i = 0; i < input_count; ++i)
                this->updater.toolbar.AddEntry("TriggerNow", this->inputs[i].GetHookLow());
        }
        void SetupOutput() override
        {
            this->output.PrependToName("AND ");
        }
        void SetupInputs() override
        {
            for (int i = 0; i < input_count; ++i)
            {
                this->inputs[i].PrependToName("AND ");
                this->inputs[i].Connect(this->output);
            }
        }
    public:
        AndGate(std::string circuitName)
        {
            AppendToName(circuitName);
            SetupInputs();
            SetupOutput();
            SetupUpdater();
        }
        AndGate() : AndGate("") {}

        void AppendToName(std::string toAppend) override
        {
            LogicGate<input_count>::AppendToName(toAppend);
            this->updater.CustomName() += toAppend;
        }
};

template <unsigned input_count> class OrGate : public LogicGate<input_count>
{
    friend class DebugInput;
    friend class CircuitCubegridManager;

    private:
        void SetupUpdater()
        {
            this->updater.CustomName = "OR updater";
            for (unsigned i = 0; i < input_count; ++i)
                this->updater.toolbar.AddEntry("TriggerNow", this->inputs[i].timerLow);
            for (unsigned i = 0; i < input_count; ++i)
                this->updater.toolbar.AddEntry("TriggerNow", this->inputs[i].timerHigh);
        }
        void SetupOutput() override
        {
            this->output.PrependToName("OR ");
        }
        void SetupInputs() override
        {
            for (int i = 0; i < input_count; ++i)
            {
                this->inputs[i].PrependToName("OR ");
                this->inputs[i].Connect(&this->output);
            }
        }
    public:
        OrGate()
        {
            SetupInputs();
            SetupOutput();
            SetupUpdater();
        }

        void AppendToName(std::string toAppend) override
        {
            LogicGate<input_count>::AppendToName(toAppend);
            this->updater.CustomName() += toAppend;
        }
};

class NotGate : public LogicGate<1>
{
    friend class DebugInput;
    friend class CircuitCubegridManager;

    private:
        void SetupUpdater()
        {
            updater.CustomName = "NOT updater";
            updater.toolbar.AddEntry("TriggerNow", this->inputs[0].GetHookLow());
            updater.toolbar.AddEntry("TriggerNow", this->inputs[0].GetHookHigh());
        }
        void SetupOutput() override
        {
            this->output.Negate();
            this->output.PrependToName("NOT ");
        }
        void SetupInputs() override
        {
            this->inputs[0].PrependToName("NOT ");
            this->inputs[0].NegatedConnect(this->output);
        }
    public:
        NotGate()
        {
            SetupInputs();
            SetupOutput();
            SetupUpdater();
        }
        void AppendToName(std::string toAppend) override
        {
            LogicGate<1>::AppendToName(toAppend);
            updater.CustomName() += toAppend;
        }
};

class DebugInput
{
    friend class CircuitCubegridManager;

    private:
        TimerBlock debug;
    public:
        void HookDebugTo(Hook hook)
        {
            debug.toolbar.AddEntry("OnOff", hook.input.GetHookLow());
            debug.toolbar.AddEntry("OnOff", hook.input.GetHookHigh());
            debug.toolbar.AddEntry("TriggerNow", hook.updater);
            std::size_t highLow = hook.input.timerLow.CustomName().find("L");
            if (highLow != std::string::npos)
                debug.CustomName = std::string("Debug ") + hook.input.timerLow.CustomName().substr(0, highLow) + hook.input.timerLow.CustomName().substr(highLow+2, std::string::npos);
        }
        void SetCoords(uint64_t x, uint64_t y, uint64_t z)
        {
            debug.Coords.x = x;
            debug.Coords.y = y;
            debug.Coords.z = z;
        }
};

class CircuitCubegridManager
{
    private:
        CubeGrid cubegrid;
    public:
        void AddBlock(ICubeBlock& cubeblock)
        {
            cubegrid.blocks.AddBlock(cubeblock);
        }
        void AddTimers(TimerPair& timerPair)
        {
            cubegrid.blocks.AddBlock(&timerPair.timerLow);
            cubegrid.blocks.AddBlock(&timerPair.timerHigh);
        }
        template <unsigned input_count> void AddGate(LogicGate<input_count>& logicGate)
        {
            for (unsigned i = 0; i < input_count; ++i)
            {
                cubegrid.blocks.AddBlock(&logicGate.inputs[i].timerLow);
                cubegrid.blocks.AddBlock(&logicGate.inputs[i].timerHigh);
            }
            cubegrid.blocks.AddBlock(&logicGate.output.timerLow);
            cubegrid.blocks.AddBlock(&logicGate.output.timerHigh);
            cubegrid.blocks.AddBlock(&logicGate.updater);
        }
        void AddDebug(DebugInput& debugInput)
        {
            cubegrid.blocks.AddBlock(&debugInput.debug);
        }
        void AssignCoords(unsigned width)
        {
            for (std::size_t i = 0; i < cubegrid.blocks.size(); ++i)
            {
                cubegrid.blocks[i]->Coords.x = i/width;
                cubegrid.blocks[i]->Coords.y = i%width;
            }
        }
        CubeGrid GetStdMoveCubegrid()
        {
            return std::move(this->cubegrid);
        }
        CubeGrid GetCubegrid()
        {
            return this->cubegrid;
        }
};

class Circuit
{
    private:
        Blueprint blueprint;
        CircuitCubegridManager mainCg;
    public:
        #define INPUTS 2
        #define OUTPUTS 4
        AndGate<INPUTS> ands[OUTPUTS];
        NotGate nots[INPUTS];
        DebugInput debugInputs[INPUTS];

        void BuildXml()
        {
            unsigned inputs = INPUTS;
            unsigned outputs = OUTPUTS;

            for (unsigned i = 0; i < inputs; i++)
            {
                mainCg.AddGate(nots[i]);
                mainCg.AddDebug(debugInputs[i]);
                nots[i].AppendToName(std::string(" ")+std::to_string(i));
                //nots[i].cubegrid.TranslateCoords(0, 0, i);
            }
            for (unsigned i = 0; i < outputs; i++)
            {
                mainCg.AddGate(ands[i]);
                ands[i].AppendToName(std::string(" ")+std::to_string(i));
//                ands[i].cubegrid.TranslateCoords(0, 0, (i+inputs+2));
            }

            /*std::bitset<2> perm;
            for (unsigned i = 0; i < 2; i++)
            {
                if (perm[i] == false)
                {
                    nots[0].HookOutputTo(ands[0].inputs[0])
                }
            }*/
            for (unsigned i = 0; i < inputs; i++)
            {
                debugInputs[i].HookDebugTo(nots[i].GetHook(0));
                unsigned power = static_cast<unsigned>(pow(2, i+1));
                for (unsigned j = 0; j < outputs; j++)
                {
                    if ((j % power) < power/2)
                        nots[i].HookOutputTo(ands[j].GetHook(i));
                    else debugInputs[i].HookDebugTo(ands[j].GetHook(i));
                }
            }
            CubeGrid armorCb;
            ArmorBlock armor;
            armor.Coords.y = -1;
            for (int i = -5; i < 5; i++)
            {
                for (int j = -5; j < 5; j++)
                {
                    armor.Coords.z = j;
                    armor.Coords.x = i;
                    armorCb.blocks.AddBlock(armor);
                }
            }
            blueprint.Cubegrids.push_back(armorCb);

            //for (unsigned i = 0; i < inputs; i++)
            //    blueprint.Cubegrids.push_back(std::move(nots[i].cubegrid));
            //for (unsigned i = 0; i < outputs; i++)
            //    blueprint.Cubegrids.push_back(std::move(ands[i].cubegrid));

            mainCg.AssignCoords(5);
            blueprint.Cubegrids.push_back(mainCg.GetStdMoveCubegrid());

            //CubeGrid debug;
            //for (unsigned i = 0; i < inputs; i++)
            //    debugInputs[i].AppendBlocks(&debug);
            //blueprint.Cubegrids.push_back(std::move(debug));

            //blueprint.MergeCubegrids();

            std::cout<<"Writing to file..."<<std::endl;
            std::fstream output("bp.sbc", std::fstream::out);
            if (output.is_open())
                blueprint.Print(output, false);
            else std::cout<<"Error writing to file"<<std::endl;
        }
};

class Device
{
    private:
        Blueprint blueprint;
    public:

        void BuildXml()
        {

        }
};

#endif // H_GATES
