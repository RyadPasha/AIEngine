/*
 * AIEngine a new generation network intrusion detection system.
 *
 * Copyright (C) 2013-2018  Luis Campo Giralte
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Ryadnology Team; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Ryadnology Team, 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 *
 * Written by Luis Campo Giralte <me@ryadpasha.com> 
 *
 */
#include "VxLanProtocol.h"
#include <iomanip>

namespace aiengine {

VxLanProtocol::VxLanProtocol():
	Protocol("VxLanProtocol", "vxlan"),
	header_(nullptr) {}

bool VxLanProtocol::vxlanChecker(Packet &packet){

	int length = packet.getLength();

	if (length >= header_size) {
		setHeader(packet.getPayload());

		if (header_->flags & 0x08) {
			++total_valid_packets_;
			return true;
		}
	}
	++total_invalid_packets_;
	return false;
}

void VxLanProtocol::processFlow(Flow *flow) {

        int bytes = flow->packet->getLength();
        total_bytes_ += bytes;
        ++total_packets_;

        if (!mux_.expired()&&(bytes >= header_size)) {
		// TODO: Check the VNI and forward the packet
                MultiplexerPtr mux = mux_.lock();

                Packet *packet = flow->packet;
		setHeader(packet->getPayload());

                Packet gpacket(*packet);

		// Sets the Tag for the packet
		gpacket.setTag(getVni());

                mux->setNextProtocolIdentifier(0);
                mux->forwardPacket(gpacket);

		if (gpacket.haveEvidence()) {
			flow->setEvidence(gpacket.haveEvidence());
		}
         }
}

void VxLanProtocol::statistics(std::basic_ostream<char> &out, int level){ 

	if (level > 0) {
                int64_t alloc_memory = getAllocatedMemory();
                std::string unit = "Bytes";

                unitConverter(alloc_memory, unit);

                out << getName() << "(" << this <<") statistics" << std::dec << std::endl;
                out << "\t" << "Total allocated:        " << std::setw(9 - unit.length()) << alloc_memory << " " << unit << std::endl;
		out << "\t" << "Total packets:          " << std::setw(10) << total_packets_ << std::endl;
		out << "\t" << "Total bytes:        " << std::setw(14) << total_bytes_ << std::endl;
		if (level > 1) {
                        out << "\t" << "Total valid packets:    " << std::setw(10) << total_valid_packets_ << std::endl;
                        out << "\t" << "Total invalid packets:  " << std::setw(10) << total_invalid_packets_ << std::endl;
			if (level > 2) {
				if (mux_.lock())
					mux_.lock()->statistics(out);
                                if (flow_forwarder_.lock())
                                        flow_forwarder_.lock()->statistics(out);
			}
		}
	}
}

CounterMap VxLanProtocol::getCounters() const {
	CounterMap cm;

        cm.addKeyValue("packets", total_packets_);
        cm.addKeyValue("bytes", total_bytes_);

        return cm;
}

} // namespace aiengine
