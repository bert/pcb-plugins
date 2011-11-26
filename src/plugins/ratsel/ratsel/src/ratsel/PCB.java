/* 
 Copyright (C) 2008 Endre Bak <endrebak@yahoo.com>

 Licensed under the terms of the GNU General Public License, version
 2 or later.

*/

package ratsel;

import java.io.FileNotFoundException;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Set;
import java.util.SortedSet;
import java.util.TreeMap;
import java.util.TreeSet;

public class PCB {

	private static NetList cNetList = new NetList();
	
	private static Pin[] cPinA;
	
	private static Rat[] cRatA;
	
	private static TreeMap<Location,Pin> cLocPinMap = new TreeMap<Location,Pin>();
	
	private static TreeMap<String,Pin> cPinNameMap = new TreeMap<String,Pin>();
	
	private static TreeMap<Connection.Key,Connection> cConnectionMap = 
		new TreeMap<Connection.Key,Connection>();
	
	private static SortedSet<String> cSelectedPinSet;
	
	public static void importLayout() {
		cleanLayout();
		importNetList();
		cPinA = JIFace.getPins();
		trc("\nPins:\n");
		for(Pin pin: cPinA) trc(pin+"\n");
		cRatA = JIFace.getRats();
		for(Pin pin: cPinA) {
			Pin conflictingPin = cLocPinMap.get(pin);
			if (conflictingPin != null) {
				warn(pin+" conflicts with "+conflictingPin+", rejecting!!!");
			} else {
				cLocPinMap.put(pin, pin);
				cPinNameMap.put(pin.getName(), pin);
			}
		}
		// collect pins for rat endpoints
		for(Rat rat: cRatA) {
			for(int i = 0; i < 2; i++) {
				Location loc = rat.getLocation(i);
				Pin pin = cLocPinMap.get(loc);
				if (pin == null)
					warn("No pin for rat location["+i+"]:"+loc);
				else
					pin.addRat(rat);
				rat.setPin(i, pin);
			}
		}
		trc("\nRats:\n");
		for(Rat rat: cRatA) trc(rat+"\n");
	}
	
	public static void ratSel(String pFileName) throws FileNotFoundException, IOException {
		trc("\nratSel("+pFileName+")\n");
		cSelectedPinSet = Parser.parseFile(pFileName);
		trc("\nselectedPins:\n"+toStr(cSelectedPinSet));
		for(String pinName: cSelectedPinSet) {
			Net net = cNetList.getNet(pinName);
			if (net == null) {
				warn("pin:"+pinName+" is not connected to any net!");
				continue;
			}
			if (net.getPin(pinName) == null)
				setPins(net);
			net.addSelectedPinName(pinName);
		}
		findConnections();
		selectConnections();
	}
	
	private static void selectConnections() {
		SortedSet<String> pinNameSet = new TreeSet<String>();
		for(Connection con: cConnectionMap.values()) {
			trc("con:"+con);
			for(Rat rat: con.getRats()) {
				trc("selectRat("+rat.getId()+")");
				pinNameSet.add(rat.getPin(0).getName());
				pinNameSet.add(rat.getPin(1).getName());
				JIFace.selectRat(rat.getId());
			}
		}
		// check
		trc("\nselection check...\n");
		for(String pinName: cSelectedPinSet) {
			if (!pinNameSet.contains(pinName)) {
				warn("Selected pin:"+pinName+" is not reached by a selected rat!");
			}
		}
	}
	
	private static void findConnections() {
		for(Net net: cNetList.getNets()) {
			for(String selPinName: net.getSelectedPinNames()) {
				Pin selPin = net.getPin(selPinName);
				ArrayList<Rat> ratList = new ArrayList<Rat>();
				followRats(ratList, net, selPin, selPin);
			}
		}
	}
	
	private static void followRats(ArrayList<Rat> pRatList, Net pNet, Pin pSrcPin, Pin pPin) {
		for(Rat rat: pPin.getRats()) {
			pRatList.add(rat);
			for(Pin ratPin: rat.getPins()) {
				if (ratPin == null || ratPin.isTraversed() || pSrcPin.equals(ratPin) || pPin.equals(ratPin))
					continue;
				ratPin.setTraversed(true);
				if (pNet.isSelectedPin(ratPin.getName())) {
					addConnection(pSrcPin, ratPin, pRatList);
				} else {
					followRats(pRatList, pNet, pSrcPin, ratPin);
				}
			}
			pRatList.remove(pRatList.size()-1);
		}
	}
	
	private static void addConnection(Pin pPin0, Pin pPin1, ArrayList<Rat> pRatList) {
		// copy ratList
		ArrayList<Rat> ratList = new ArrayList<Rat>(pRatList);
		Connection con = new Connection(new Pin[] { pPin0, pPin1 }, ratList);
		trc("addConnection("+con+")");
		Connection oldCon = cConnectionMap.get(con.getKey());
		if (oldCon == null || con.getLength() < oldCon.getLength()) {
			trc("connection added");
			cConnectionMap.put(con.getKey(), con);
		}
	}
	
	
	private static void msg(String pMsg) {
		JIFace.trc(pMsg);
	}
	
	private static void trc(String pMsg) {
		//msg("trc:"+pMsg+"\n");
	}
	
	private static void warn(String pMsg) {
		msg("Warning:"+pMsg+"\n");
	}
	
	private static void setPins(Net pNet) {
		for(String pinName: pNet.getPinNames()) {
			Pin pin = cPinNameMap.get(pinName);
			if (pin == null) throw new IllegalArgumentException(
				"net:"+pNet.getName()+" contains undefined pin:"+pinName+"!"
			);
			pNet.addPin(pin);
		}
	}
	
	private static void importNetList() {
		int netN = JIFace.getNetNameN();
		for(int netIdx=0; netIdx<netN; netIdx++) {
			String netName = JIFace.getNetName(netIdx).trim();;
			int pinN = JIFace.getPinNameN(netIdx);
			for(int pinIdx=0; pinIdx<pinN; pinIdx++) {
				String pinName = JIFace.getPinName(netIdx, pinIdx).trim();
				cNetList.addPinName(netName, pinName);
			}
		}
		trc("netList:\n"+cNetList);
	}
	
	/**
	 * Cleans the data which could reamin from the previus call.
	 */
	private static void cleanLayout() {
		cNetList.clear();
		cLocPinMap.clear();
		cPinNameMap.clear();
		cConnectionMap.clear();
	}
	
	private static String toStr(Set<String> pSet) {
		StringBuilder bldr =  new StringBuilder();
		for(String str: pSet) {
			bldr.append(" -> "+ str + "\n");
		}
		return bldr.toString();
	}
	
}