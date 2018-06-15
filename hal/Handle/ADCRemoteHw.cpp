#include "hal/Handle/ADCRemoteHw.h"
#include "hal/backend/RemoteADCBackend.h"

namespace HMF {
namespace Handle {

ADCRemoteHw::ADCRemoteHw(Coordinate::IPv4 const& host, Coordinate::TCPPort const& port, HMF::ADC::USBSerial const& adc)
    : ADC(adc),
      rcfInit(new RCF::RcfInitDeinit),
      adc_client(new RcfClient<I_HALbeADC>(RCF::TcpEndpoint(host.to_string(), port))),
      m_host(host),
      m_port(port)
{
	// use boost::serialization for marshalling
	adc_client->getClientStub().setSerializationProtocol(RCF::Sp_BsBinary);
	// 256MB seems to be the AnaRM buffer limit => 512MiB is safe :)
	adc_client->getClientStub().getTransport().setMaxMessageLength(512*1024*1024);
	// enforce connection NOW
	adc_client->getClientStub().connect();
}

ADCRemoteHw::ADCRemoteHw() : rcfInit(new RCF::RcfInitDeinit) {}

ADCRemoteHw::~ADCRemoteHw() {}

HMF::ADC::USBSerial ADCRemoteHw::boardId() const
{
	return m_usbserial;
}

Coordinate::IPv4 ADCRemoteHw::host() const
{
	return m_host;
}

Coordinate::TCPPort ADCRemoteHw::port() const
{
	return m_port;
}

template<typename Archiver>
void ADCRemoteHw::serialize(Archiver& ar, const unsigned int)
{
	using namespace boost::serialization;
	ar & make_nvp("m_usbserial", m_usbserial);
	// FIXME: use serialize of Coordinate::IPv4!
	unsigned long host_as_ulong = m_host.to_ulong();
	ar & make_nvp("host_as_ulong", host_as_ulong);
	m_host = HMF::Coordinate::IPv4(host_as_ulong);
	ar & make_nvp("m_port", m_port);
}

boost::shared_ptr<ADCRemoteHw> createADCRemoteHw()
{
	boost::shared_ptr<ADCRemoteHw> ptr{new ADCRemoteHw{}};
	return ptr;
}

boost::shared_ptr<ADCRemoteHw> createADCRemoteHw(Coordinate::IPv4 const& host,
                                                 Coordinate::TCPPort const& port,
                                                 HMF::ADC::USBSerial const& adc)
{
	boost::shared_ptr<ADCRemoteHw> ptr{new ADCRemoteHw{host, port, adc}};
	return ptr;
}

void freeADCRemoteHw(ADCRemoteHw& handle)
{
	// shutdown connection to RCF server (by deleting the client)
	handle.adc_client.reset();
}

} // Handle
} // HMF

BOOST_CLASS_EXPORT_IMPLEMENT(::HMF::Handle::ADCRemoteHw)

#include "boost/serialization/serialization_helper.tcc"
EXPLICIT_INSTANTIATE_BOOST_SERIALIZE(::HMF::Handle::ADCRemoteHw)
