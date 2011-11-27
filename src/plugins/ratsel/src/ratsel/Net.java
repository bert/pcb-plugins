/* 
 Copyright (C) 2008 Endre Bak <endrebak@yahoo.com>

 Licensed under the terms of the GNU General Public License, version
 2 or later.

*/

package ratsel;

import java.util.ArrayList;
import java.util.Set;
import java.util.TreeMap;

public class Net {
	
	private String iName;
	
	private TreeMap<String, Pin> iPinNameMap = new TreeMap<String, Pin>();
	
	private ArrayList<String> iSelecetdPinNames = new ArrayList<String>();
	
	public Net(String pNetName) {
		iName = pNetName;
	}
	
	public String getName() {
		return iName;
	}
	
	public void addPinName(String pPinName) {
		iPinNameMap.put(pPinName, null);
	}
	
	public void addSelectedPinName(String pPinName) {
		iSelecetdPinNames.add(pPinName);
	}
	
	/**
	 * @param pPin
	 */
	public void addPin(Pin pPin) {
		String pinName = pPin.getName();
		/*
		if (!iPinNameMap.containsKey(pinName)) throw new IllegalArgumentException(
			"Net:"+iNetName+" doesn't contain pin:"+pinName+"!"
		);
		*/
		iPinNameMap.put(pinName, pPin);
	}
	
	public boolean hasPin(String pPinName) {
		return iPinNameMap.containsKey(pPinName);
	}
	
	public boolean isSelectedPin(String pPinName) {
		return iSelecetdPinNames.contains(pPinName);
	}
	
	public Pin getPin(String pPinName) {
		return iPinNameMap.get(pPinName);
	}
	
	public Set<String> getPinNames() {
		return iPinNameMap.keySet();
	}
	
	public ArrayList<String> getSelectedPinNames() {
		return iSelecetdPinNames;
	}
	
	public boolean equals(Net pNet) {
		return iName.equals(pNet.iName);
	}
	
	public String toString() {
		StringBuilder bldr = new StringBuilder(iName+":\n -> ");
		boolean more = false;
		for(String pinName: iPinNameMap.keySet()) {
			if (more) bldr.append(" ,"); else more = true;
			bldr.append(pinName);
		}
		bldr.append('\n');
		return bldr.toString();
	}
	
}