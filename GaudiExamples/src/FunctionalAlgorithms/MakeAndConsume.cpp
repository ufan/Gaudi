#include <GaudiKernel/AnyDataHandle.h>
#include <GaudiAlg/Producer.h>
#include <GaudiAlg/Consumer.h>
#include <GaudiAlg/Transformer.h>
#include <GaudiKernel/MsgStream.h>


namespace Gaudi { namespace Examples {

class IntDataProducer : public Gaudi::Functional::Producer<int()> {
public:
    IntDataProducer(const std::string& name, ISvcLocator* svcLoc)
             : Producer( name, svcLoc,
               KeyValue("OutputLocation", {"MyInt"})) {}

    int operator()() const override {
        info() << "executing IntDataProducer, storing 7 into " << outputLocation() << endmsg;
        return 7;
    }


};

DECLARE_COMPONENT(IntDataProducer)

class IntDataConsumer : public Gaudi::Functional::Consumer<void(const int&)> {
public:
    IntDataConsumer(const std::string& name, ISvcLocator* svcLoc)
             : Consumer( name, svcLoc,
               KeyValue("InputLocation", {"MyInt"})) {}

    void operator()(const int& input) const override {
        info() << "executing IntDataConsumer, consuming " << input
               << " from " << inputLocation() << endmsg;
    }


};

DECLARE_COMPONENT(IntDataConsumer)

class IntToFloatData : public Gaudi::Functional::Transformer<float(const int&)> {
public:
    IntToFloatData(const std::string& name, ISvcLocator* svcLoc)
            : Transformer(name, svcLoc,
              KeyValue("InputLocation", {"MyInt"}),
              KeyValue("OutputLocation", {"MyFloat"})) {}

    float operator() (const int& input) const override {
      info() << "Converting: " << input << " from " << inputLocation()
             << " and storing it into " << outputLocation() << endmsg;
        return float(input);
    }
};

DECLARE_COMPONENT(IntToFloatData)

class IntIntToFloatFloatData : public Gaudi::Functional::MultiTransformer
    <std::tuple<float, float>(const int&, const int&)> {
public:
    IntIntToFloatFloatData(const std::string& name, ISvcLocator* svcLoc)
            : MultiTransformer(name, svcLoc,
                               {KeyValue("InputLocation1", {"MyInt"}),
                                KeyValue("InputLocation2", {"MyOtherInt"})},
                               {KeyValue("OutputLocation1", {"MyMultiFloat1"}),
                                KeyValue("OutputLocation2", {"MyMultiFloat2"})}) {}

    std::tuple<float, float> operator() (const int& input1, const int& input2) const override {
        info() << "Number of inputs : " << inputLocationSize()
               << ", number of outputs : " << outputLocationSize() << endmsg;
        info() << "Converting " << input1 << " from " << inputLocation<0>()
               << " and " << input2 << " from " << inputLocation<1>() << endmsg;
        info() << "Storing results into " << outputLocation<0>()
               << " and " << outputLocation<1>() << endmsg;
        return std::make_tuple(float(input1), float(input2));
    }
};

DECLARE_COMPONENT(IntIntToFloatFloatData)

class FloatDataConsumer : public Gaudi::Functional::Consumer<void(const float&)> {
public:
    FloatDataConsumer(const std::string& name, ISvcLocator* svcLoc)
             : Consumer( name, svcLoc,
               KeyValue("InputLocation", {"MyFloat"})) {}

    void operator()(const float& input) const override {
        info() << "executing FloatDataConsumer: " << input << endmsg;
    }
};

DECLARE_COMPONENT(FloatDataConsumer)
}}


