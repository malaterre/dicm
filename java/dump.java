import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.util.ArrayList;
import java.util.List;

import javax.xml.namespace.QName;
import javax.xml.stream.XMLEventReader;
import javax.xml.stream.XMLInputFactory;
import javax.xml.stream.XMLStreamException;
import javax.xml.stream.XMLStreamConstants;
import javax.xml.stream.events.Attribute;
import javax.xml.stream.events.EndElement;
import javax.xml.stream.events.StartElement;
import javax.xml.stream.events.XMLEvent;

public class dump {

    public static void main(String[] args) {
        String fileName = args[0];
        parseXML(fileName);
    }

    private static void parseXML(String fileName) {
        XMLInputFactory xmlInputFactory = XMLInputFactory.newInstance();
        try {
            XMLEventReader xmlEventReader = xmlInputFactory.createXMLEventReader(new FileInputStream(fileName));
            while(xmlEventReader.hasNext()) {
                System.out.println("before" );
 /*
 *  Bulk reading occur in nextEvent(), not in hasdNext():
javax.xml.stream.XMLStreamException: ParseError at [row,col]:[15,13]
Message: Content is not allowed in trailing section.
	at java.xml/com.sun.org.apache.xerces.internal.impl.XMLStreamReaderImpl.next(XMLStreamReaderImpl.java:652)
	at java.xml/com.sun.xml.internal.stream.XMLEventReaderImpl.nextEvent(XMLEventReaderImpl.java:83)
	at dump.parseXML(dump.java:29)
	at dump.main(dump.java:20)
 */
                XMLEvent xmlEvent = xmlEventReader.nextEvent();
                System.out.println("after" );
		switch( xmlEvent.getEventType() ) {
			case XMLStreamConstants.START_ELEMENT:
				System.out.println( "START_ELEMENT" );
				break;
			case XMLStreamConstants.END_ELEMENT:
				System.out.println( "END_ELEMENT" );
				break;
			case XMLStreamConstants.CHARACTERS:
				System.out.println( "CHARACTERS" );
				break;
			case XMLStreamConstants.ATTRIBUTE:
				System.out.println( "ATTRIBUTE" );
				break;
			case XMLStreamConstants.NAMESPACE:
				System.out.println( "NAMESPACE" );
				break;
			case XMLStreamConstants.PROCESSING_INSTRUCTION:
				System.out.println( "PROCESSING_INSTRUCTION" );
				break;
			case XMLStreamConstants.COMMENT:
				System.out.println( "COMMENT" );
				break;
			case XMLStreamConstants.START_DOCUMENT:
				System.out.println( "START_DOCUMENT" );
				break;
			case XMLStreamConstants.END_DOCUMENT:
				System.out.println( "END_DOCUMENT" );
				break;
			case XMLStreamConstants.DTD:
				System.out.println( "DTD" );
				break;
			default:
				System.out.println( "DEFAULT" );
		}
           }
        } catch (FileNotFoundException | XMLStreamException e) {
            e.printStackTrace();
        }
    }
}
