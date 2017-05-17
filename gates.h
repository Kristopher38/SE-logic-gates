#ifndef H_GATES
#define H_GATES

#include <fstream>
#include <string>
#include <bitset>
#include <cmath>
#include "blueprintlib/blueprint.h"
#include "blueprintlib/blocks.h"

class CircuitCubegridManager;

class TimerPair
{
    friend class DebugInput;
    friend class CircuitCubegridManager;

    public:
        TimerBlock timerLow;
        TimerBlock timerHigh;
        BlockGroup toSwitchHighGroup;
        BlockGroup toSwitchLowGroup;
        BlockGroup toUpdateGroup;

    public:
        bool useGroups;
        enum TIMER {LOW = 0, HIGH = 1};

        void UpdateGroupNames()
        {
            toSwitchHighGroup.name = timerHigh.CustomName() + std::string(" Group");
            toSwitchLowGroup.name = timerLow.CustomName() + std::string(" Group");
            toUpdateGroup.name = timerHigh.CustomName() + std::string(" Updater Group");
        }
        TimerPair(bool _useGroups = false)
        {
            useGroups = _useGroups;
            timerLow.Enabled = true;
            timerHigh.Enabled = false;
            timerLow.CustomName = "L";
            timerHigh.CustomName = "H";
            this->UpdateGroupNames();
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
            this->UpdateGroupNames();
        }
        void PrependToName(std::string toPrepend)
        {
            timerLow.CustomName().insert(0, toPrepend);
            timerHigh.CustomName().insert(0, toPrepend);
            this->UpdateGroupNames();
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
            if (this->useGroups)
            {
                toSwitchLowGroup.AddBlock(toSwitch.timerLow);
                toSwitchHighGroup.AddBlock(toSwitch.timerHigh);
                timerLow.toolbar.AddEntry(negate ? "OnOff_Off" : "OnOff_On", toSwitchLowGroup, 0);
                timerLow.toolbar.AddEntry(negate ? "OnOff_On" : "OnOff_Off", toSwitchHighGroup, 1);
                timerHigh.toolbar.AddEntry(negate ? "OnOff_On" : "OnOff_Off", toSwitchLowGroup, 0);
                timerHigh.toolbar.AddEntry(negate ? "OnOff_Off" : "OnOff_On", toSwitchHighGroup, 1);
            } else {
                timerLow.toolbar.AddEntry(negate ? "OnOff_Off" : "OnOff_On", toSwitch.timerLow);
                timerLow.toolbar.AddEntry(negate ? "OnOff_On" : "OnOff_Off", toSwitch.timerHigh);
                timerHigh.toolbar.AddEntry(negate ? "OnOff_On" : "OnOff_Off", toSwitch.timerLow);
                timerHigh.toolbar.AddEntry(negate ? "OnOff_Off" : "OnOff_On", toSwitch.timerHigh);
            }
        }
        void AddUpdate(TimerPair& toUpdate)
        {
            if (this->useGroups)
            {
                toUpdateGroup.AddBlock(toUpdate.timerLow);
                toUpdateGroup.AddBlock(toUpdate.timerHigh);
                timerLow.toolbar.AddEntry("TriggerNow", toUpdateGroup, 2);
                timerHigh.toolbar.AddEntry("TriggerNow", toUpdateGroup, 2);
            } else {
                timerLow.toolbar.AddEntry("TriggerNow", toUpdate.timerLow);
                timerLow.toolbar.AddEntry("TriggerNow", toUpdate.timerHigh);
                timerHigh.toolbar.AddEntry("TriggerNow", toUpdate.timerLow);
                timerHigh.toolbar.AddEntry("TriggerNow", toUpdate.timerHigh);
            }
        }
        void AddUpdate(TimerBlock& toUpdate)
        {
            if (this->useGroups)
            {
                toUpdateGroup.AddBlock(toUpdate);
                timerLow.toolbar.AddEntry("TriggerNow", toUpdateGroup, 2);
                timerHigh.toolbar.AddEntry("TriggerNow", toUpdateGroup, 2);
            } else {
                timerLow.toolbar.AddEntry("TriggerNow", toUpdate);
                timerHigh.toolbar.AddEntry("TriggerNow", toUpdate);
            }
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

    public:
        TimerPair inputs[input_count];
        TimerPair output;
        Updater updater;

        LogicGate(bool useGroups = false)
        {
            SetupInputs(useGroups);
            SetupOutput(useGroups);
            SetupUpdater();
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

        virtual void SetupOutput(bool useGroups)
        {
            output.SetCoords(input_count, 0, 0, TimerPair::LOW);
            output.SetCoords(input_count, 1, 0, TimerPair::HIGH);
            output.PrependToName("output ");
            output.useGroups = useGroups;
        }

        virtual void SetupInputs(bool useGroups)
        {
            for (int i = 0; i < input_count; ++i)
            {
                inputs[i].SetCoords(i, 0, 0, TimerPair::LOW);
                inputs[i].SetCoords(i, 1, 0, TimerPair::HIGH);
                inputs[i].PrependToName(std::string("input ") + this->GenerateLetter(i) + std::string(" "));
                output.useGroups = useGroups;
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
            updater.CustomName() += toAppend;
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
        void SetupUpdater() override
        {
            this->updater.CustomName = "AND updater";
            for (unsigned i = 0; i < input_count; ++i)
                this->updater.toolbar.AddEntry("TriggerNow", this->inputs[i].GetHookHigh());
            for (unsigned i = 0; i < input_count; ++i)
                this->updater.toolbar.AddEntry("TriggerNow", this->inputs[i].GetHookLow());
        }
        void SetupOutput(bool useGroups) override
        {
            this->output.useGroups = useGroups;
            this->output.PrependToName("AND ");
        }
        void SetupInputs(bool useGroups) override
        {
            for (int i = 0; i < input_count; ++i)
            {
                this->inputs[i].useGroups = useGroups;
                this->inputs[i].PrependToName("AND ");
                this->inputs[i].Connect(this->output);
            }
        }
    public:
        AndGate(bool useGroups = false)
        {
            SetupInputs(useGroups);
            SetupOutput(useGroups);
            SetupUpdater();
        }
};

template <unsigned input_count> class OrGate : public LogicGate<input_count>
{
    friend class DebugInput;
    friend class CircuitCubegridManager;

    private:
        void SetupUpdater() override
        {
            this->updater.CustomName = "OR updater";
            for (unsigned i = 0; i < input_count; ++i)
                this->updater.toolbar.AddEntry("TriggerNow", this->inputs[i].timerLow);
            for (unsigned i = 0; i < input_count; ++i)
                this->updater.toolbar.AddEntry("TriggerNow", this->inputs[i].timerHigh);
        }
        void SetupOutput(bool useGroups) override
        {
            this->output.useGroups = useGroups;
            this->output.PrependToName("OR ");
        }
        void SetupInputs(bool useGroups) override
        {
            for (int i = 0; i < input_count; ++i)
            {
                this->inputs[i].useGroups = useGroups;
                this->inputs[i].PrependToName("OR ");
                this->inputs[i].Connect(&this->output);
            }
        }
    public:
        OrGate(bool useGroups = false)
        {
            SetupInputs(useGroups);
            SetupOutput(useGroups);
            SetupUpdater();
        }
};

class NotGate : public LogicGate<1>
{
    friend class DebugInput;
    friend class CircuitCubegridManager;

    private:
        void SetupUpdater() override
        {
            updater.CustomName = "NOT updater";
            updater.toolbar.AddEntry("TriggerNow", this->inputs[0].GetHookLow());
            updater.toolbar.AddEntry("TriggerNow", this->inputs[0].GetHookHigh());
        }
        void SetupOutput(bool useGroups) override
        {
            this->output.useGroups = useGroups;
            this->output.Negate();
            this->output.PrependToName("NOT ");
        }
        void SetupInputs(bool useGroups) override
        {
            this->inputs[0].useGroups = useGroups;
            this->inputs[0].PrependToName("NOT ");
            this->inputs[0].NegatedConnect(this->output);
        }
    public:
        NotGate(bool useGroups = true)
        {
            SetupInputs(useGroups);
            SetupOutput(useGroups);
            SetupUpdater();
        }
};

class InputGate : public LogicGate<1>
{
    friend class DebugInput;
    friend class CircuitCubegridManager;

    private:
        void SetupUpdater() override
        {
            updater.CustomName = "INPUT updater";
            updater.toolbar.AddEntry("TriggerNow", this->inputs[0].GetHookLow());
            updater.toolbar.AddEntry("TriggerNow", this->inputs[0].GetHookHigh());
        }
        void SetupOutput(bool useGroups) override
        {
            this->output.useGroups = useGroups;
            this->output.PrependToName("INPUT ");
        }
        void SetupInputs(bool useGroups) override
        {
            this->inputs[0].useGroups = useGroups;
            this->inputs[0].PrependToName("INPUT ");
            this->inputs[0].Connect(this->output);
        }
    public:
        InputGate(bool useGroups = true)
        {
            SetupInputs(useGroups);
            SetupOutput(useGroups);
            SetupUpdater();
        }
};

class DebugInput
{
    friend class CircuitCubegridManager;

    public:
        TimerBlock debugTimer;
        BlockGroup debugGroupInput;
        BlockGroup debugGroupUpdater;
    public:
        void HookDebugTo(Hook hook)
        {
            debugGroupInput.AddBlock(hook.input.timerHigh);
            debugGroupInput.AddBlock(hook.input.timerLow);
            debugGroupUpdater.AddBlock(hook.updater);
            debugTimer.toolbar.AddEntry("OnOff", debugGroupInput, 0);
            debugTimer.toolbar.AddEntry("TriggerNow", debugGroupUpdater, 1);
            /*std::size_t highLow = hook.input.timerLow.CustomName().find("L");
            if (highLow != std::string::npos)
                debugTimer.CustomName = std::string("Debug ") + hook.input.timerLow.CustomName().substr(0, highLow) + hook.input.timerLow.CustomName().substr(highLow+2, std::string::npos);*/
        }
        void SetCoords(uint64_t x, uint64_t y, uint64_t z)
        {
            debugTimer.Coords.x = x;
            debugTimer.Coords.y = y;
            debugTimer.Coords.z = z;
        }
        void SetName(std::string name)
        {
            debugTimer.CustomName = name;
            debugGroupInput.name = name + std::string(" inputs");
            debugGroupUpdater.name = name + std::string(" updaters");
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
            if (timerPair.toSwitchLowGroup.size())
                cubegrid.groups.push_back(timerPair.toSwitchLowGroup);
            if (timerPair.toSwitchHighGroup.size())
                cubegrid.groups.push_back(timerPair.toSwitchHighGroup);
            if (timerPair.toUpdateGroup.size())
                cubegrid.groups.push_back(timerPair.toUpdateGroup);
        }
        template <unsigned input_count> void AddGate(LogicGate<input_count>& logicGate)
        {
            for (unsigned i = 0; i < input_count; ++i)
            {
                cubegrid.blocks.AddBlock(&logicGate.inputs[i].timerLow);
                cubegrid.blocks.AddBlock(&logicGate.inputs[i].timerHigh);
                if (logicGate.inputs[i].toSwitchLowGroup.size())
                    cubegrid.groups.push_back(logicGate.inputs[i].toSwitchLowGroup);
                if (logicGate.inputs[i].toSwitchHighGroup.size())
                    cubegrid.groups.push_back(logicGate.inputs[i].toSwitchHighGroup);
                if (logicGate.inputs[i].toUpdateGroup.size())
                    cubegrid.groups.push_back(logicGate.inputs[i].toUpdateGroup);
            }
            cubegrid.blocks.AddBlock(&logicGate.output.timerLow);
            cubegrid.blocks.AddBlock(&logicGate.output.timerHigh);
            cubegrid.blocks.AddBlock(&logicGate.updater);
            if (logicGate.output.toSwitchLowGroup.size())
                cubegrid.groups.push_back(logicGate.output.toSwitchLowGroup);
            if (logicGate.output.toSwitchHighGroup.size())
                cubegrid.groups.push_back(logicGate.output.toSwitchHighGroup);
            if (logicGate.output.toUpdateGroup.size())
                cubegrid.groups.push_back(logicGate.output.toUpdateGroup);
        }
        void AddDebug(DebugInput& debugInput)
        {
            cubegrid.blocks.AddBlock(&debugInput.debugTimer);
            if (debugInput.debugGroupInput.size())
                cubegrid.groups.push_back(debugInput.debugGroupInput);
            if (debugInput.debugGroupUpdater.size())
                cubegrid.groups.push_back(debugInput.debugGroupUpdater);
        }
        std::size_t AssignCoords(unsigned width)
        {
            std::size_t i;
            for (i = 0; i < cubegrid.blocks.size(); ++i)
            {
                cubegrid.blocks[i]->Coords.x = i/width;
                cubegrid.blocks[i]->Coords.y = i%width;
            }
            return i/width;
        }
        CubeGrid GetStdMoveCubegrid()
        {
            return std::move(this->cubegrid);
        }
        CubeGrid& GetCubegrid()
        {
            return this->cubegrid;
        }
        void TranslateCoords(int64_t x, int64_t y, int64_t z)
        {
            cubegrid.TranslateCoords(x, y, z);
        }
};

template <unsigned input_count, unsigned output_count>
class Decoder
{
    private:
        CircuitCubegridManager mainCg;
        AndGate<input_count+1> ands[output_count];
        NotGate nots[input_count];
        InputGate inputs[input_count];
        InputGate enable;
        DebugInput debugInputs[input_count];
        InteriorLight outputLights[output_count];
        InteriorLight inputLights[input_count];
    public:
        Decoder(std::string name)
        {
            for (unsigned i = 0; i < input_count; i++)
            {
                inputs[i].AppendToName(std::string(" ")+name+std::string(" ")+std::to_string(i));
                nots[i].AppendToName(std::string(" ")+name+std::string(" ")+std::to_string(i));
                debugInputs[i].SetName(std::string("Debug input ") + name + std::string(" ") + std::to_string(i));
                inputLights[i].CustomName = std::string(" ")+name+std::string("Light in "+std::to_string(i));

                debugInputs[i].debugTimer.toolbar.AddEntry("OnOff", inputLights[input_count-i-1], 2);
                debugInputs[i].HookDebugTo(inputs[i].GetHook(0));
            }
            for (unsigned i = 0; i < output_count; i++)
            {
                outputLights[i].CustomName = std::string(" ")+name+std::string("Light out "+std::to_string(i));
                ands[i].AppendToName(std::string(" ")+name+std::string(" ")+std::to_string(i));
                ands[i].output.timerLow.toolbar.AddEntry("OnOff_Off", outputLights[i]);
                ands[i].output.timerHigh.toolbar.AddEntry("OnOff_On", outputLights[i]);
                enable.HookOutputTo(ands[i].GetHook(input_count));
                mainCg.AddGate(ands[i]);
            }
            enable.AppendToName(std::string(" ")+name+" ENABLE");
            mainCg.AddGate(enable);
            for (unsigned i = 0; i < input_count; i++)
            {
                inputs[i].HookOutputTo(nots[i].GetHook(0));
                unsigned power = static_cast<unsigned>(pow(2, i+1));
                for (unsigned j = 0; j < output_count; j++)
                {
                    if ((j % power) < power/2)
                        nots[i].HookOutputTo(ands[j].GetHook(i));
                    else inputs[i].HookOutputTo(ands[j].GetHook(i));
                }
                mainCg.AddDebug(debugInputs[i]);
                mainCg.AddGate(nots[i]);
                mainCg.AddGate(inputs[i]);
            }
            /*std::size_t length = */mainCg.AssignCoords(25);

            /*CubeGrid armorCb;
            ArmorBlock armor;
            armor.Coords.y = -1;
            for (int i = 0; i < length; i++)
            {
                for (int j = 0; j < 4; j++)
                {
                    armor.Coords.z = j;
                    armor.Coords.x = i;
                    armorCb.blocks.AddBlock(armor);
                }
            }

            armor.Coords.z = 2;
            for (int i = 0; i < 8; i++)
            {
                for (int j = 0; j < 8; j++)
                {
                    armor.Coords.y = j;
                    armor.Coords.x = i;
                    armorCb.blocks.AddBlock(armor);

                    outputLights[i*8+j].Coords.y = i;
                    outputLights[i*8+j].Coords.z = 3;
                    outputLights[i*8+j].Coords.x = j;
                    outputLights[i*8+j].BlockOrientation.Forward = ORIENT_BACKWARD;
                    outputLights[i*8+j].BlockOrientation.Up = ORIENT_DOWN;
                    outputLights[i*8+j].Enabled = false;
                    armorCb.blocks.AddBlock(&outputLights[i*8+j]);
                }

            }
            outputLights[0].Enabled = true;

            armor.Coords.z = 2;
            for (int i = 0; i < inputs; i++)
            {
                for (int j = 8; j < 10; j++)
                {
                    armor.Coords.y = j;
                    armor.Coords.x = i;
                    armorCb.blocks.AddBlock(armor);
                }
            }
            for (int i = 0; i < inputs; i++)
            {
                inputLights[i].Coords.y = 9;
                inputLights[i].Coords.z = 3;
                inputLights[i].Coords.x = i;
                inputLights[i].BlockOrientation.Forward = ORIENT_BACKWARD;
                inputLights[i].BlockOrientation.Up = ORIENT_DOWN;
                inputLights[i].Enabled = false;
                armorCb.blocks.AddBlock(&inputLights[i]);
            }

            blueprint.Cubegrids.push_back(std::move(armorCb));*/

        }
        CubeGrid GetStdMoveCubegrid()
        {
            return std::move(mainCg.GetStdMoveCubegrid());
        }
        CubeGrid& GetCubegrid()
        {
            return mainCg.GetCubegrid();
        }
        void HookOutputTo(unsigned output_index, Hook hook)
        {
            if (output_index >= output_count)
                throw std::out_of_range("Output index out of range");
            else
                ands[output_index].HookOutputTo(hook);
        }
        virtual Hook GetHook(unsigned inputIndex)
        {
            if (inputIndex > input_count)
                throw std::out_of_range("Input index out of range");
            else if (inputIndex == input_count)
                return enable.GetHook(0);
            else
                return inputs[inputIndex].GetHook(inputIndex);
        }
        void TranslateCoords(int64_t x, int64_t y, int64_t z)
        {
            mainCg.TranslateCoords(x, y, z);
        }
};

class Device
{
    private:
        Blueprint blueprint;
        Decoder<6,64> decoder6to64[4] = {{"DEC64-0"}, {"DEC64-1"}, {"DEC64-2"}, {"DEC64-3"}};
        Decoder<2,4> decoder2to4 = {"DEC4-0"};
    public:

        void BuildXml()
        {
            //decoder6to64.TranslateCoords();
            for (unsigned i = 0; i < 4; i++)
            {
                decoder2to4.HookOutputTo(i, decoder6to64[i].GetHook(6));
                decoder2to4.GetCubegrid().AttachCubegrid(decoder6to64[i].GetStdMoveCubegrid(), 0, 0, i+1);
            }


            blueprint.Cubegrids.push_back(decoder2to4.GetStdMoveCubegrid());
            //for (unsigned i = 0; i < 4; i++)
            //    blueprint.Cubegrids.push_back(decoder6to64[i].GetStdMoveCubegrid());
            std::cout<<"Writing to file..."<<std::endl;
            std::fstream output("bp.sbc", std::fstream::out);
            if (output.is_open())
                blueprint.Print(output, false);
            else std::cout<<"Error writing to file"<<std::endl;
        }
};

#endif // H_GATES
