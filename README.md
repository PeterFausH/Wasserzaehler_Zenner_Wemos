[hier gehts zum Blogeintrag:](https://nc-x.com/monitoring-wasserzaehler-zenner)

# Wasserzaehler_Zenner_Wemos
using a magnetometer HMC5883L and a Wemos D1 mini R3 to monitor our water consumption
Völlig unbemerkt haben wir im Dorf diesesmal Wasserzähler mit einem verbauten Magneten im Zählwerk erhalten. Dies ist die Vorstufe einer Schnittstelle. Für viel Geld kann dann darauf aufbauend ausgelesen werden. Viel Geld muss ich nicht ausgeben, geht viel günstiger.

# Daten erfassen
Im Prinzip brauchen wir etwas, das den Vorbeiflug des Magneten im Zählwerk registriert. Das könnte ein Reed-Kontakt sein. Ich nehme einen digitalen Kompass, ein Magnetometer. Ist wegen der Drohnen ein Massenartikel und kostet gerademal 3,- Euro. Die Signale werte ich mit einem Wemos D1 Mini aus, den gibts im 3er Pack für 4,- Euro/Stück. Im Gegensatz zum Reed-Kontakt könnte ich auch Teilumdrehungen aus den Änderungen des Magnetfeldes errechnen.
![Lötvorbereitung](https://github.com/user-attachments/assets/ff9bdbe9-0124-41eb-a139-521157360011)

Nach Datenblatt gibt mein Wasserzähler alle 10 Liter einen Impuls. Das wäre dann wenn der Magnet dem Sensor am nächsten ist. Sicher kann ich aus herausfinden wann der Magnet dem Magnetometer am entferntesten ist, dann hätte ich ein zweites Signal für je 5 Liter.
# Daten auswerten
Der ganze Stack mit InfluxDB und Grafana läuft schon im Haus, eine Klimadaten-Datenbank ist auch schon angelegt, so muss ich diesbezüglich nichts mehr vorbereiten. 
# Hardware vorbereiten
Die Wemos D1 Mini kommen mit verschiedenen Headern zum anlöten. Am Magnetometer, einem HMC5883L, löte ich die beiliegende Stiftleiste an. Dazu nutze ich immer ein Breadboard um schön winklig ausrichten zu können.
![Header-Auswahl](https://github.com/user-attachments/assets/1fca5983-7439-4042-bd78-e7445167ad12)

Diesesmal entscheide ich mich für die kleinste Bauform und nehme die untersten Header. Diese löte ich so ein, dass die WLAN-Antenne oben ist und die Stifte nach unten abgehen. Am Magnetometer biege ich die Beinchen um alles nachher in einen Schrumpfschlauch zu bekommen:
![Platz_sparen_Beinchen_biegen](https://github.com/user-attachments/assets/744fa2d3-bff5-4ede-8536-f68e90fbdfb0)

Wir brauchen 4 Leitungen. Am einfachsten ist GND, das geht einfach auf beiden Seiten an die entsprechend beschrifteten Pins. Dann VCC an 3V3 für die Spannungsversorgung. SCL geht an D1 und SDA geht an D2. DRDY ist unbenutzt.
![Verdrahtung_Magnetometer_WemosD1](https://github.com/user-attachments/assets/c227aef7-e033-4b1e-9df6-bd7bf98959db)

Um ungewünschte Kontakte am Gussgehäuse des Wasserzählers zu vermeiden kommt ein Schrumpfschlauch drüber. Ich nehme zwei Größen, einer passend für die 4 Leitungen und einer in den gerade so das Magnetometer geht.Ich erwärme traditionell mit einem Feuerzeug. Den kleinen Überstand vorne am Magnetometer presse ich mit einer Zange zusammen.
![Magnetometer_im_Schrumpfschlauch](https://github.com/user-attachments/assets/c9854434-2cef-4ada-b073-05a305b27536)

Die Kabel gehen waagerecht ab, so konzentriere ich mich auf das Magnetfeld das in Y-Richtung wirkt. 
![Magnetometer_Y-Achse_Vorbeiflug](https://github.com/user-attachments/assets/d18a1a76-7db8-4ca1-a0d5-f8549eadad9a)


# Versuchsaufbau
Draussen am Wasserzähler ist es kalt und ich kann nur im Stehen arbeiten mit einem kippligen Stand für den Laptop. Deshalb habe ich mir mit fischertechnik geholfen, das stand noch von den letzten Enkelbasteleien rum. Ein kleiner Motor und eine Rotation mit einem aufgeklebten Magneten ist alles was ich brauche. Damit kann ich den Vorbeiflug des Magneten am Magnetometer simulieren.

# jetzt zum Code:
Es geht darum die vom Sensor ausgegebenen Y-Werte so zu interpretieren dass ich die Stellen finde, an denen sich die Messwerte wenden. Also die Stellen an denen sich der Magnet bis auf ein Maximum an den Sensor annähert und dann wieder entfernt. 

Es gibt noch einen zweiten Extremwert, der an dem sich der Sensor vom Magneten wegbewegt und danach wieder auf ihn zubewegt. Diese Wendepunkte sind die Crossings der gedachten Achse die vom Sensor durch den Drehpunkt des Messrades geht. 

Da ich zwei Sensoren in zwei verschiedenen Häusern montieren will, möchte ich nicht jedesmal diese Extremstellen suchen müssen. Der Code sucht diese in den ersten paar Umdrehungen selbst und nutzt diese dann weiter anstatt dass feste Werte vergeben werden. Ebenso berücksichtige ich, ob das Sensorrad links oder rechtsrum dreht, es ist dem Code egal.

Damit mir der Wemos D1 die Influxdatenbank nicht mit immergleichen Werten zumüllt, meldet er nur, wenn ein Crossing stattfand und wenn die letze Meldung schon eine Weile her ist. Momentan steht publish_delay auf 10 Sekunden weil ich kein Signal verpassen möchte. Dadurch kommen fast immer 5 Liter-Meldungen.

![Auswahl_20241222_005](https://github.com/user-attachments/assets/f031b4af-077f-4e35-aa8b-e438d68977c3)

Klar ist das keine Echtzeitmeldung, wenn zum Beispiel nach dem letzten Vorbeiflug (crossing) nur noch 3 Liter verbraucht wurden, werden die erst mit der nächsten 5 Liter-Meldung ausgegeben. Das kann 3 Stunden später sein. Ganz ehrlich, mir egal, ich wusste nichtmal wieviel Wasser wir fürs Duschen brauchen, wieviel sich die Waschmaschine nimmt und wieviel wann für die Spülmaschine draufgeht. Das werde ich jetzt alles untersuchen.
