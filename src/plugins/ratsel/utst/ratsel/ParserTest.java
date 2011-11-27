/* 
 Copyright (C) 2008 Endre Bak <endrebak@yahoo.com>

 Licensed under the terms of the GNU General Public License, version
 2 or later.

*/

package ratsel;

import java.util.SortedSet;

import junit.framework.TestCase;

public class ParserTest extends TestCase {
	
	private static final String DATA = 
		"\n\nR1-2 ,  R2-1, R3-(1), Opa2-(2,3)\n"+ 
		"Opa3-(1-5), IC3-( 1, 4,6 -10, 12 ), R20-1\n";
	
	private static final String[] PINNAMES = {
		"R1-2", "R2-1", "R3-1", "Opa2-2", "Opa2-3",
		"Opa3-1", "Opa3-2", "Opa3-3", "Opa3-4", "Opa3-5",
		"IC3-1", "IC3-4", "IC3-6", "IC3-7", "IC3-8", "IC3-9", "IC3-10", "IC3-12",
		"R20-1"
	};
	
	public void testParse() {
		SortedSet<String> pinSet = Parser.parse(DATA);
		assertTrue(
			"Size mismatch! pinSet.size()="+pinSet.size()+
			", PINNAMES.length="+PINNAMES.length,
			pinSet.size() == PINNAMES.length
		);
		for(String pinName: pinSet) {
			assertTrue(
				"pinName:"+pinName+" is not contained in pinSet!", pinSet.contains(pinName)
			);
		}
	}
	
}