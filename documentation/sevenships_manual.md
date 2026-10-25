# Seven Ships of the Enemy — Player's Manual

*First draft, 25 October 2026.*

> *"My father, behold, the enemy's ships came; my cities were burned, and
> they did evil things in my country. Does not my father know that all my
> troops and chariots are in the land of Hatti, and all my ships are in the
> land of Lukka? … Seven ships of the enemy came, and they did great damage."*
>
> — Ammurapi, last king of Ugarit, to the king of Alashiya, about 1185 BC

## 1. The setting

The eastern Mediterranean, 1600 BC. Four great powers share the coasts and
the river valleys between Greece and the Nile: palace kingdoms that trade
copper, tin, cedar, wine and purple dye with each other, write to each other
as brothers, and go to war when the letters turn cold.

You lead one of them for four hundred years, from the rise of the palaces to
the end of the Bronze Age. Around 1200 BC the world they built breaks
apart: raiders come from the sea, the trade routes fail, droughts and
earthquakes strike, and kingdoms that seemed eternal fall into ruin.
**The game is about what you build before that, and what survives it.**

It plays like *Civilization*: you found cities, work the land around them,
build units, explore, trade, fight and make peace, turn by turn, on a map
of hexagons.

### The four peoples

| People | Ruler | Starts near | Advantage |
|---|---|---|---|
| **Mycenaeans** | Agamemnon, Wanax of Mycenae | wine or olives | Ships cost a quarter less |
| **Hittites** | Suppiluliuma, Great King of Hatti | copper | Chariots cost a quarter less |
| **Egyptians** | Thutmose III, King of Egypt | grain | Cities need a quarter less food to grow |
| **Canaanites** | Niqmaddu II, King of Ugarit | cedar or purple dye | Buying production costs a third less (mercenaries), and each trade deal brings a gold more |

You choose one; the computer leads the other three. Every people is in every
game. There are also **barbarians**, who own no kingdom but raid everybody,
and late in the game the **Sea Peoples**.

## 2. Starting a game

The title screen offers **Start Game**, **Load Saved Game** and **Quit**.
Start Game asks for your people, then the difficulty level:

| Level | What the computer players pay for what they build and for their cities' growth | Barbarians |
|---|---|---|
| **Scribe** | 150% (and you start with 50 gold) | rare, small bands |
| **Governor** | 120% | occasional |
| **Prince** | 100% — a fair contest | occasional |
| **King** | 80% | frequent |
| **Great King** | 70% | frequent, large bands |

On harder levels the computer players also think less well of you, and
anger more easily.

You begin with a **settler** and a **warrior**. Found your first city
quickly: press **b** with the settler.

## 3. The screen

- **The map** takes most of the screen. Unexplored land is black until one
  of your units or cities comes near it.
- **The minimap**, top left, shows the whole world; the white frame is
  the part you are looking at. Click it to jump there.
- **The panel**, down the left, shows your people, the year, the turn,
  your gold, the unit waiting for orders and its moves left, and messages.
- **The readout**, top right, names whatever is under the pointer: the
  terrain, its resource and improvements, what it yields (food/shields/
  trade), and its position — or the unit or city on it.
- **The menu bar**, top: Game, Orders, Advisors, World, Help.

The world wraps east to west, but not north to south.

## 4. Controls

Everything can be done from the keyboard; a mouse or joystick moves the
pointer, and the button acts as a click.

### Moving and orders

| Key | Action |
|---|---|
| **Tab** | Tile select: a ring on the unit; the cursor keys move it, Return moves the unit there (or acts on that hexagon) |
| **click** | Next to the unit: move there (onto an enemy: attack; onto your galley: board) |
| **g** | Go to: pick a hexagon with the ring or a click, and the unit walks there over the coming turns |
| **b** | Found a city (settler); found an emporium (merchant ship) |
| **f** | Fortify (a galley on a merchant ship's hexagon escorts it) |
| **i** | Irrigate (builder); fish (fishing boat on fish) |
| **m** | Build a mine (builder) |
| **p** | Build a pasture on cattle or horses (builder) |
| **r** | Build a road (builder) |
| **w** | Wait: ask again after the other units |
| **space** | Skip this unit's turn |
| **c** | Centre the map on the unit |
| **Return** | End the turn (when the panel asks) |

### The map and the game

| Key | Action |
|---|---|
| **cursor keys** | Scroll the map |
| **click** | On a city of yours: the city screen. On a unit of yours: give it the orders. Elsewhere: centre the map there |
| **F1 – F4** | The advisors: Cities, Military, Diplomacy, Finance |
| **MEGA + letter** | Open a menu (MEGA+G for Game, and so on) |
| **HELP** | The list of keys |
| **Esc** | Quit to BASIC (after asking) |

In the xemu emulator, HELP is on **PageUp**, MEGA is the left **Alt** key,
and the keypad's 7 9 4 6 1 3 move the unit.

By default the turn ends by itself once every unit has had its orders;
Game → Options → End of Turn makes it wait for Return instead.

## 5. Cities

A settler founds a city where it stands (**b**). A city stands on a road,
and is the centre of its **reach**: its own hexagon and two rings around
it, 19 hexagons. Where the reaches of two cities overlap, a hexagon
belongs to the nearer city, so cities founded close together share their
land; the computer players keep theirs four hexagons apart.

### Citizens and growth

A city of size *n* works its own hexagon and *n* hexagons of its reach,
the best ones, unless you choose them yourself on the city screen.
**Each citizen eats 2 food.** What is left over goes into the food box;
when the box is full (10 × (size + 1)) the city grows, and when it is
empty and still short of food, the city shrinks. Without irrigation, most
cities stop growing where the land can feed no more.

### Production

The **shields** of the worked hexagons go towards whatever the city is
building. When there are enough, the unit appears in the city, or the
building is put up. A **settler takes a citizen** with it, so it waits
until the city is size 2.

### The city screen

Click one of your cities (or Return on it in tile select) to open it:

- **Left**: the units in the city. Click one, or press its number, to wake
  it (or to fortify it).
- **Middle**: the city and its reach. A yellow ring marks a worked
  hexagon. Click a hexagon (or move to it with the cursor keys and press
  space) to take a citizen off it or put an idle one on it.
- **Below**: size, food, shields, trade, how soon it grows, its wonders.
- **Right**: what it is producing. **CHANGE** (c) lists what it can build;
  **BUY** (b) finishes it at the end of the turn for gold. Under them, the
  city's buildings.

**Advisors → Cities** (F1) lists all your cities; pick one to open it.

## 6. The land

### Terrain

| Terrain | Food | Shields | Trade | Defence |
|---|---|---|---|---|
| Grassland | 2 | 0 (half of it 1) | 0 | |
| Plains | 1 | 1 | 0 | |
| Floodplain | 3 | 0 (half of it 1) | 1 | |
| Forest | 1 | 2 | 0 | ×1.5 |
| Hills | 1 | 0 | 0 | ×1.5 |
| Mountains | 0 | 1 | 0 | ×2 |
| Desert | 0 | 1 | 0 | |
| Tundra | 1 | 0 | 0 | |
| Shallows | 1 | 0 | 2 | |
| Ocean | 1 | 0 | 2 | |

The **floodplain** is the land along the rivers: the grain of the world.
Help → Terrain Types has a page on each.

### Resources

| Resource | Where | Adds |
|---|---|---|
| Grain | floodplain, grassland | 2 food |
| Fish | shallows | 2 food |
| Cattle | grassland, plains, hills | 1 food, 1 shield |
| Olives | hills | 1 food, 2 trade |
| Wine | hills, plains | 3 trade |
| Cedar | forest | 2 shields |
| Copper | hills, mountains | 2 shields |
| Tin | hills, mountains; and at the far north and south | 1 shield |
| Purple dye | shallows | 3 trade |
| Horses | grassland, plains | 2 shields |

A resource in the reach of a city adds to what its hexagon yields.

**Your people *has* a resource** when it is on one of your cities, or in
a city's reach and improved the way it needs — copper and tin **mined**,
cattle and horses with a **pasture**, olives and wine **irrigated** —
or when you receive it in trade, or it is next to one of your emporia.
Grain, fish, cedar and dye need nothing.

What you have decides what you can build:

- **Bronze** — copper *and* tin — for the **spearman**. Tin is scarce: a few
  hills and mountains, and the edges of the world. Most peoples have to
  trade for it.
- **Horses** for the **chariot**.
- **Cedar**, and a city on the coast, for the **galley**.

### Improvements

Builders improve the land. A builder at work asks for no orders until it
is done; several builders on one hexagon work together.

| Improvement | Key | Turns | Where | Adds |
|---|---|---|---|---|
| Irrigation | i | 5 | grassland, plains, desert, floodplain, hills, next to water (the sea, a river's floodplain, other irrigation) | 1 food |
| Mine | m | 10 | hills, mountains, desert; copper and tin anywhere | 3 / 2 / 1 shields on hills / mountains / desert |
| Pasture | p | 5 | cattle and horses | 1 food, 1 shield |
| Road | r | 2 (4 on hills and in forest, 6 on mountains) | any land | 1 trade on grassland, plains and desert |

**A step from road to road costs a third of a move.** Every city stands on
a road.

A **fishing boat** works fish: on a fish, press **i**. The boat is used up
and the fish yields a food more for good.

## 7. Units

| Unit | Attack | Defence | Moves | Cost | Needs |
|---|---|---|---|---|---|
| Settler | 0 | 1 | 1 | 30 | a city of size 2 (takes a citizen) |
| Warrior | 1 | 1 | 1 | 10 | |
| Builder | 0 | 1 | 1 | 20 | |
| Spearman | 1 | 2 | 1 | 20 | bronze |
| Chariot | 4 | 1 | 2 | 40 | horses; never onto mountains |
| Galley | 1 | 1 | 3 | 40 | cedar, a coastal city |
| Fishing boat | 0 | 1 | 2 | 20 | a coastal city |
| Merchant ship | 0 | 1 | 3 | 30 | a coastal city |

Ships sail the **shallows** only, and their own cities. **A galley carries
two land units**: step onto it to board; they are woken when it stops next
to land, to step ashore. The fishing boat explores the coasts and works
fish. The merchant ship carries trade and founds emporia (section 10).

**Go to** (**g**): pick a hexagon, and the unit walks there by itself over
the explored land (or a ship over the shallows), turn after turn, and asks
for orders when it arrives or finds the way blocked.

**Advisors → Military** (F2) counts your units by kind, what they cost
you, and your wars.

## 8. Combat

Moving onto another people's unit or city is an attack — the game asks
first if it means declaring war. The attacker wins with a chance of
attack against defence: a chariot (4) attacking a warrior (1) wins four
times in five. The defender's strength is multiplied by its ground:

- hills and forest ×1.5, mountains ×2;
- fortified, or in a city, ×1.5;
- in the city with the Lion Gate, ×2;
- a galley fortified at sea (an escort), ×2.

The best defender on a hexagon fights. Outside a city, if it loses, every
unit on the hexagon dies with it; in a city only the defender dies. **A
city is taken by moving into it when nobody is left in it**; barbarians
burn it instead.

Every fight you are in is shown to you, with the map on it.

## 9. Diplomacy

You **meet** a people when one of your units or cities comes next to one
of theirs. The rulers then write to you, in the words of the Amarna
letters: *"Say to Agamemnon, Wanax of Mycenae, my brother: thus says
Suppiluliuma, Great King of Hatti, your brother…"*

Between any two peoples there is **war**, a **ceasefire** (16 turns, then
war again unless peace is made), or **peace**. Every people starts at peace
with every other, and at war with the barbarians.

Each ruler has an **attitude** to you — hostile, cold, wary, cordial,
friendly — which drifts back to its temper over time, and which you
change: wars declared on it or on its friends, your units by its cities,
your cities crowding its own, refused offers and tribute all make it
colder; peace and trade make it warmer. A cold and strong ruler declares
war; one losing a war asks for a ceasefire; one at ceasefire and friendly
offers peace. Letters asking something are answered **Yes** or **No**.

**Advisors → Diplomacy** (F3) lists every people: how you stand, its ruler,
its cities and army, its attitude and its wars and trade. Choose a people
with the cursor keys, then:

| Key | Proposal |
|---|---|
| C | a ceasefire (at war) |
| P | peace (in a ceasefire) |
| W | declare war |
| T | trade, or end a trade (at peace) |
| S | demand submission (at war) |

### Vassals

A people at war that is losing badly — weaker than its enemy, with half
its cities or fewer — may **submit** rather than fight on, in the
vassals' formula: *"At the feet of my lord I fall, seven times and seven
times."* You can accept, or demand it yourself (S). A vassal is at peace
with its lord, pays it **an eighth of its gold every turn** as tribute,
and counts for its lord's score. It goes free when it grows stronger than
its lord, or its lord falls. You are never anybody's vassal.

## 10. Trade

### Goods and deals

The trade goods are **olives, wine, cedar, copper, tin, purple dye and
horses**. A **deal** is one good sent for another, between two peoples at
peace, or a gift. A good is not used up by sending it, and **a good
received counts as had** — traded tin and your own copper make bronze.
Partners think better of each other, and each deal brings both of them
gold every turn.

Rulers write to ask for trades, and strong, cold ones demand goods as
tribute. You propose a trade on the Diplomacy screen (T).

### Merchant fleets

**A deal lapses 40 turns after the last merchant ship came** — unless the
two peoples have cities within 8 hexagons of each other, and trade over
land. To keep a deal alive, load a merchant ship in one of your cities and
sail it next to (or into) a city of your partner's. Raiders and enemies
sink merchant ships, so **escort them**: a galley fortified (**f**) on the
merchant ship's hexagon goes where it goes and defends it twice as well.

### Emporia

A merchant ship can also found an **emporium** (**b**): a trading post on
a foreign coast, next to the ship, by trade goods, at least two hexagons
from any city. The ship is used up. **Your people owns the goods on the
six hexagons around it** — tin and copper for bronze, cedar for galleys,
the rest for trade and score. An enemy standing on an emporium at the end
of a turn burns it.

## 11. The treasury

Every turn, **each city's trade, less its corruption, goes into your
gold**, and your trade deals add more. Corruption grows with the distance
from your capital (your first city); without a capital, almost all the
trade is lost.

What you pay:

- **the upkeep of your city buildings**: 1 gold a turn for a temple or an
  edubba, 2 for a feasting hall, nothing for a stele (short of gold, a
  building is sold);
- **1 gold a turn for each unit** beyond the free ones: 4, and 3 more for
  each city (short of gold, a unit is disbanded);
- **a growing cost for a realm of more than 10 cities**;
- **your scribes** (below).

**Gold buys production**: BUY on the city screen finishes it at the end of
the turn. The price rises steeply for more shields, and is doubled if
nothing has been built towards it yet.

**Advisors → Finance** (F4) shows it all, city by city.

### Scribes and the clay tablets

On the Finance screen, **+ and −** set what you pay your **scribes** each
turn. Each gold paid is a point of **knowledge** — and each edubba
(section 12) adds a point a turn and a fifth of the pay more — and at
100, 300 and 600 points your scribes set down, in turn:

| Tablet | Knowledge | Effect |
|---|---|---|
| **Linear B** | 100 | Your cities lose a quarter less to corruption |
| **A law code** | 300 | Five more cities before the realm costs gold |
| **The alphabet** | 600 | Your trade deals bring twice the gold |

Each is worth legacy (section 12): 10 points, 20 for the first people in
the world to set it down.

## 12. Culture and legacy

**Legacy points** are what a people leaves behind. They are yours for
good, whatever becomes of your cities, and they count in the score and
decide the **legacy victory**.

| Source | Legacy |
|---|---|
| A temple | 3 when built |
| A city building (a temple too) | 1 every 32 turns, while you hold its city (see below) |
| A wonder | 20 |
| A clay tablet | 10 (20 if the first in the world) |
| Purple dye, owned or traded for | 1 every 16 turns |

### City buildings

Each city can have one of each. **They add to the legacy of whoever holds
the city, every 32 turns, for as long as they stand**, so the earlier they
are built the more they leave — the way to a hegemony as well as to a
legacy victory. Short of gold for the upkeep, one is sold.

| Building | Cost | Upkeep | Legacy every 32 turns | Also |
|---|---|---|---|---|
| **Temple** | 40 | 1 | 1 | 3 legacy when built |
| **Stele** | 30 | none | 1, 2 while at war with a people | |
| **Edubba** (scribal school) | 60 | 1 | 1 | a point of knowledge a turn, and a fifth of the scribes' pay more |
| **Feasting Hall** | 60 | 2 | 2 | |

Help → City Buildings describes them.

### The wonders

Each can be built once in the world, in one city. When one is built,
everybody hears of it, and nobody else can build it. Its effect is for
**whoever holds its city**.

| Wonder | Cost | Effect |
|---|---|---|
| **The Lion Gate** of Mycenae | 160 | Every unit in its city defends twice as well |
| **The Palace of Knossos** | 200 | Half the corruption in all its holder's cities |
| **The Temple of Karnak** | 200 | Its holder's temples cost nothing and give twice the legacy |
| **The Great Harbour of Ugarit** | 160 | Its holder's ships have a move more (a coastal city only) |

Help → Wonders describes them.

## 13. Barbarians

From about turn 30 bands of barbarian warriors appear on empty land far
from anybody and attack whatever is near — units, cities, builders. They
burn the cities they take. Keep a guard in every city; more on harder
levels.

## 14. The calendar

The game runs from **1600 BC** to **1026 BC**: two years a turn until
1250 BC (turn 176), then one, **400 turns** in all. World → Score shows
how many are left.

## 15. The Crisis

Around **1200 BC** the Late Bronze Age ends.

- **Omens.** About turn 200 a rumour of trouble in the west; at 215 a
  warning from another ruler; at **turn 226** the letter from Ugarit:
  *seven ships of the enemy came…*
- **The Sea Peoples.** From then on fleets of raiders — fierce fighters,
  attack 3 — come over the sea to the richest coastal cities, more and
  more often towards the end. They attack from their ships, land, and
  burn what they take.
- **Network fracture.** Raiders sink merchant ships, and the trade deals
  they carried lapse; lose the copper or tin you depended on, and bronze
  goes with it.
- **Shocks.** Droughts (dry land yields less food for a while),
  earthquakes (a city shrinks and loses an improvement), plague (the
  largest city loses a third of its people), and revolts (an unguarded
  city goes over to rebels).
- **Pleas.** Rulers who lose cities write to you for help that cannot
  come.

Coastal cities need more guards, merchant ships need escorts, and a
treasury and a second source of copper and tin are worth more than ever.

### The collapse

**A computer people that loses its capital breaks apart**: if it has three
or more cities, its cities furthest from the largest — about a third of its
people — rise in revolt and become rebel cities, barbarian-held, which keep
a guard and can be taken. This can happen at any time, but is most likely
in the Crisis.

## 16. The end: winning and losing

The game ends after **turn 400**, or when every other people has fallen,
or when you have no cities and no units left.

The **score** of each people is:

- 1 point for each citizen,
- 2 for each city,
- 2 for each trade good it has,
- 3 for each trade deal,
- 10 for each vassal,
- 1 for every 200 gold,
- and its legacy points.

The outcome, judged in this order:

1. **Hegemony** — your people stands at the end with the highest score of
   all the peoples still standing.
2. **Legacy** — you have more legacy points than any other people, fallen
   or not. This victory can be won **posthumously**: even if your cities
   fall in the Collapse, what you built is remembered.
3. **Survival** — your people stands at the end. Many did not.
4. **Defeat** — your people has fallen, and others left more behind.

**Game → Retire** ends the game early, with the score as it stands.
World → Score shows the table at any time.

## 17. Saving

There are ten places to save a game in, numbered 0 to 9. **Game → Save
Game** lists them, each with its comment or "empty": pick one with the
cursor keys and Return, its number, or the mouse, and type a comment to
remember the game by. It starts as your leader, your people and the year
("Agamemnon - Mycenaeans - 1450 BC"); DEL rubs out, Return saves and Esc
goes back to the list. A save in a place that holds one replaces it.
**Load Saved Game** on the title screen, or **Game → Load Game** during
a game, lists the same ten and picks up the one you choose; the game you
were playing is lost unless you saved it. A save made by another version of the game is listed
as "from another version" and cannot be loaded.

## 18. Some advice

- Found your first city at once, and a second and third soon: a settler
  needs a city of size 2.
- Keep a defender in every city, fortified. Barbarians come from turn 30.
- Builders: irrigate grassland and floodplain, mine hills, pasture horses
  and cattle, and join your cities with roads.
- Look for copper and tin early. If you lack one, trade for it.
- Trade with your neighbours, and keep the deals alive with merchant
  ships — escorted ones after 1200 BC.
- Pay your scribes a little every turn: the tablets add up.
- Build steles and temples early, and halls and edubbas in the larger
  cities: their legacy grows with every turn they stand.
- Before the Crisis, put more guards in your coastal cities, and keep
  some gold.
