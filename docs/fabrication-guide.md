# Fabrication guide

How to build the 7-layer paper prototype of the Eco-Smart patch and test it
with artificial sweat.

> [!WARNING]
> Educational prototype, not a medical device. The materials have not been
> tested for skin safety or biocompatibility. Do bench tests on a tabletop or
> on a glove, not on broken or irritated skin. Use heat with adult supervision.

## Materials and tools

| Item | Used for |
|---|---|
| Cornstarch 20 g, water 60 mL, white vinegar 5 mL, glycerin 5 mL | Bioplastic backing |
| Gelatin 10 g, warm water 30 mL | Hydrogel pad |
| Transparent plastic sheet (e.g. a report cover or OHP film) | Protective layer and channel walls |
| Double-sided tape | Microfluidic channel spacer |
| Universal-indicator paper | pH zone |
| Copper or aluminium foil | Electrodes |
| Skin-safe adhesive tape (e.g. medical tape) | Adhesive ring |
| Saucepan, thermometer, spoon, baking paper, craft knife, ruler, hole punch, 3 cm circular cutter (or a bottle cap) | |
| Distilled water 100 mL + NaCl 0.5 g | Artificial sweat |
| Diluted white vinegar (pH 4-5), baking-soda solution (pH 8-9) | Demonstration buffers |

## 1. Bioplastic backing (layer 6), 24 h ahead

1. In a saucepan, stir **20 g cornstarch** into **60 mL cold water** until
   smooth. Add **5 mL vinegar** and **5 mL glycerin**.
2. Heat on low and stir constantly. Hold the mixture at **70-80 °C**. Within a
   few minutes it thickens and turns from opaque white to **translucent**.
   Take it off the heat as soon as it is uniformly translucent; overheating
   makes it brittle.
3. Spread it on baking paper to an even **2-3 mm**.
4. **Dry for 24 h** at room temperature, away from direct heat.
5. Peel it off and cut a backing about 1 cm larger than the patch footprint
   (e.g. a 5 × 5 cm square).

*Troubleshooting*: if it cracks, it was too thin or held too hot, so add a
little more glycerin next time. If it stays sticky, dry it longer.

## 2. Hydrogel pad (layer 3)

1. Sprinkle **10 g gelatin** onto **30 mL warm water** (about 50-60 °C, not
   boiling) and stir until fully dissolved.
2. Pour it into a flat container about 3 mm deep and leave it to set (30 min
   in the fridge).
3. Cut **3 cm discs**. Store them covered; they dry out quickly in open air.

## 3. Microfluidic zigzag channel (layer 2)

1. Cut a strip of double-sided tape to the patch size.
2. With a craft knife, cut a **zigzag channel about 2 mm wide** through the
   tape and peel out the channel. The zigzag lengthens the path, so the fluid
   front is easy to see and arrives gradually.
3. Stick the tape onto one transparent sheet, then press a second sheet on top
   to close the channel.
4. Punch an **inlet hole** over the start of the zigzag. The channel's end
   should sit over the hydrogel pad.

## 4. pH zone (layer 4)

Cut a **1 × 1 cm square of universal-indicator paper** and place it on the
hydrogel, next to the electrode gap, so wetted fluid reaches both.

## 5. Electrodes (layer 5)

1. Cut two foil strips, about **3 × 20 mm**.
2. Lay them parallel on the backing with a **2-3 mm gap** under the hydrogel.
   The gap sets the cell constant: keep it the same between builds.
3. Attach a jumper wire to each tail, with tape for aluminium or solder for
   copper. Wire one strip to **5 V** and the other to **A1**, and add a
   **1 kΩ** resistor from A1 to GND (see the wiring section of the README).

## 6. Assembly (bottom to top)

```
7  adhesive ring          ← skin side
6  bioplastic backing
5  foil electrodes (wires out to the Nano)
4  pH indicator square
3  hydrogel disc (3 cm)
2  zigzag channel (inlet hole on top)
1  transparent protective sheet
```

Press the layers together, then run the adhesive ring around the edge so the
stack is sealed except at the inlet.

## 7. Electronics

Wire the Nano as shown in the README's [wiring section](../README.md#wiring):
DS18B20 on D2 with a 4.7 kΩ pull-up, LEDs on D5 and D6 with 220 Ω resistors,
the electrode divider on A1, and an optional pH front-end on A0. Power it from
a USB power bank.

## 8. Artificial sweat test

1. Mix **100 mL distilled water + 0.5 g NaCl** (artificial sweat, about
   9 mS/cm).
2. Flash `nanoatmega328` and open `python tools/serial_plotter.py --port <port>`.
3. Wet the electrodes with a few drops at the inlet, wait for the reading to
   settle, then send **`r`** to (re)calibrate on the wetted patch. The green
   LED blinks during calibration, then stays on.
4. Observe the following:
   - the fluid front moving through the zigzag channel and into the hydrogel;
   - the indicator square changing colour;
   - conductivity on serial rising from 0 to several mS/cm.
5. Add a drop of **pH 4-5** buffer, then later **pH 8-9**, and note the
   indicator colours. Change the conductivity (a drop of more concentrated
   salt water) or the temperature (warm the DS18B20 in your fingers). After
   two anomalous windows (about 16-30 s) the **red LED** turns on. It turns
   off once the signals return to baseline.
6. Save the log and plot it:
   `python tools/serial_plotter.py --file session.csv --save session.png`.

Record what you observe (times, colours, values), but don't interpret any of
it as a health measurement.

## 9. Clean-up and disposal

The gelatin, cornstarch backing and indicator paper can go in compost or the
household waste. Remove and recycle the foil, the plastic sheets and the
electronics separately.
