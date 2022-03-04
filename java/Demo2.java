import java.io.FileReader;
import javax.xml.stream.*;
import javax.xml.stream.events.*;

public class Demo2 {

    public static void main(String[] args) throws Exception {
        XMLInputFactory factory = XMLInputFactory.newInstance();
        XMLEventReader er = factory.createXMLEventReader(new FileReader("test.xml"));
        System.out.println(er.getClass());

        while(er.hasNext()) {
            XMLEvent xmlEvent = er.nextEvent();
            int eventType = xmlEvent.getEventType();
            if (eventType == XMLStreamConstants.START_DOCUMENT) {
                System.out.println("Start Document" );
            } else if (eventType == XMLStreamConstants.END_DOCUMENT) {
                System.out.println("End Document" );
            } else if (eventType == XMLStreamConstants.END_ELEMENT) {
                System.out.println("End Element:    " + xmlEvent.asEndElement().getName());
            } else if (eventType == XMLStreamConstants.START_ELEMENT) {
                System.out.println("Start Element:  " + xmlEvent.asStartElement().getName());
            }
        }
    }

}
