/* 
 Copyright (C) 2008 Endre Bak <endrebak@yahoo.com>

 Licensed under the terms of the GNU General Public License, version
 2 or later.

*/

package ratsel;

import java.util.Collection;
import java.util.TreeMap;


/**
 * Net in this context represents an electrical node (equipotential node, see Kirchoff node rule).
 * From PCB context maybe it is better to call them nets, since they connect wires :)
 * In PCB pins which are on the same net (same potential, same Kirchoff node) are called nodes :)<br/>
 * To make life easier the plugin uses a mixed naming :) see table below:
 * <table border=1>
 * <tr><td>PCB</td><td>Kirchoff</td><td>Plugin</td></tr>
 * <tr><td>Net</td><td>Node</td><td>Net</td></tr>
 * <tr><td>Node</td><td>Pin</td><td>Pin</td></tr>
 * </table>
 */
public class NetList {
	
	// netName -> net
	private  TreeMap<String, Net> iNetMap = new TreeMap<String, Net>();
	
	// pinName -> net
	private TreeMap<String, Net> iPinMap = new TreeMap<String, Net>();
	
	public void clear() {
		iNetMap.clear(); iPinMap.clear();
	}
	
	public void addNetName(String pNetName) {
		if (iNetMap.containsKey(pNetName)) throw new IllegalArgumentException(
			"Net "+pNetName+" is already in NetList!"
		);
		iNetMap.put(pNetName, new Net(pNetName));
	}
	
	/*
	 * A net contians multiply pins. A pin is connected to only one net.
	 */
	public void addPinName(String pNetName, String pPinName) {
		// add to netMap
		Net net = iNetMap.get(pNetName);
		if (net == null) {
			net = new Net(pNetName);
			iNetMap.put(pNetName, net);
		}
		net.addPinName(pPinName);
		// add to pinMap
		Net pinNet = iPinMap.get(pPinName);
		if (pinNet != null && !pinNet.equals(net)) throw new IllegalArgumentException(
			"Pin:"+pPinName+" is already connected to net:"+pinNet.getName()+
			" and cannot be connected to net:"+pNetName+" too!"
		);
		iPinMap.put(pPinName, net);
	}

	public Collection<Net> getNets() {
		return iNetMap.values();
	}
	
	public Net getNet(String pPinName) {
		return iPinMap.get(pPinName);
	}

	public String toString() {
		StringBuilder bldr = new StringBuilder();
		for(Net net: iNetMap.values()) {
			bldr.append(net.toString());
		}
		return bldr.toString();
	}

}

