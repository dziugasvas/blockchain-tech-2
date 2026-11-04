# Supaprastinta blokų grandinė

Blokų grandinių technologijų 2-oji užduotis. C++ programa, kuri sukuria vartotojus ir transakcijas, sudeda jas į blokus, iškasa blokus (Proof-of-Work), patikrina juos ir atnaujina balansus.

## Versijos

- **v0.1** – veikianti grandinė: vartotojai, transakcijos, PoW, balansų atnaujinimas.
- **v0.2** – Merkle medis, išplėsta validacija su atmetimo priežastimis, 5 kandidatai kitam blokui, teisingumo testai.
- **v0.3** – DI įrankiu patobulinta versija: greitesnis kasimo ciklas ir daugiau testų (žr. skyrių „v0.3: patobulinimas su DI“).

## Paleidimas

Reikia `g++` su C++17 (testuota macOS, Apple clang, MacBook Air M4).

```bash
make                  # kompiliuoja
./blockchain          # visas rinkinys
./blockchain small    # mažas pavyzdys: 5 vartotojai, 20 transakcijų
make test             # teisingumo testai
make bench            # kasimo greičio matavimas
```

## Projekto struktūra

```
src/
├── hash.h / hash.cpp            maišos funkcija iš 1-ojo darbo
├── blockchain.h / blockchain.cpp duomenų struktūros, Merkle medis, kasimas, validacija, kandidatai
└── main.cpp                     duomenų generavimas, pagrindinis ciklas, išvestis
tests/
└── tests.cpp                    teisingumo testai (make test)
benchmark/
└── bench.cpp                    kasimo greičio matavimas (make bench)
Makefile
```

## Nustatymai

- 1 000 vartotojų, 10 000 transakcijų, blokas iki 100 transakcijų
- `seed = 42` (`std::mt19937`)
- PoW sudėtingumas 3: bloko maiša turi prasidėti `000`
- Sąskaitų modelis: `public_key -> balansas`, be komisinių ir kasimo atlygio
- Pradinė būsena įrašoma į `initial_state.csv` prieš pritaikant transakcijas

## Maiša

Naudoju savo 1-ojo darbo maišos funkciją – [blockchain-tech-1](https://github.com/dziugasvas/blockchain-tech-1), versija **v0.2** (256 bitai, 64 hex simboliai). Rinkausi v0.2, nes ankstesnėje versijoje buvo lengvai randamų kolizijų.

| Kas | Kaip skaičiuojama |
|---|---|
| `public_key` | `H(vardas)`, pvz. `H("User1")` |
| transakcijos ID | `H("sender\|receiver\|amount\|sequence")` |
| `tx_root` | Merkle šaknis iš bloko transakcijų ID |
| bloko maiša | `H("prev_hash\|timestamp\|version\|tx_root\|nonce\|difficulty")` |

Tekstas koduojamas UTF-8, skaičiai rašomi dešimtaine forma be tarpų ir pradinių nulių, eilutės pabaigos nėra. Laukai atskiriami `|`, kad skirtingi duomenys nesusijungtų į tą pačią eilutę (`1|23` ≠ `12|3`).

## Modelis

- **Vartotojas:** vardas (`User1`...), `public_key`, balansas nuo 100 iki 1 000 000.
- **Transakcija:** siuntėjas, gavėjas (skirtingi), suma nuo 1 iki 100 000, unikalus `sequence`, ID.
- **Blokas:** antraštė iš 6 laukų ir transakcijų sąrašas. Versija `1`.
- **Genesis:** `prev_hash` – 64 nuliai, `timestamp = 0`, `tx_root = H("")`, nekasamas (`difficulty = 0`).

## Merkle medis

Lapai – bloko transakcijų ID jų tvarka. Kiekviename lygyje du gretimi mazgai dekoduojami iš hex į baitus, sujungiami ir maišomi: `H(kairys || dešinys)`. Jei lygyje nelyginis mazgų skaičius, paskutinis poruojamas su savo kopija. Kartojama, kol lieka vienas mazgas – šaknis.

- 1 transakcija – šaknis yra pats jos ID
- 0 transakcijų (genesis) – `H("")`

```
3 transakcijos:      H( H(A||B) || H(C||C) )
                       /                \
                  H(A||B)              H(C||C)
                  /     \              /     \
                 A       B            C       C
```

## Validacija

Tikrinama trijose vietose, kiekviena atmestis turi priežastį.

**1. Prieš įtraukiant į pool** (`checkTransaction` + dublikatų patikra):
`nezinomas siuntejas`, `nezinomas gavejas`, `siuntejas ir gavejas sutampa`, `suma turi buti teigiama`, `neteisingas transakcijos ID`, `dubliuotas ID`.

**2. Sudarant kandidatą:** patvirtinti balansai nukopijuojami, transakcijos pritaikomos kopijai iš eilės. Netelpančios (`nepakankamas balansas`) į kandidatą neįtraukiamos. Tikri balansai nesikeičia.

**3. Prieš priimant iškastą bloką** (`validateBlock`):
`nepalaikoma versija`, `neteisingas prev_hash`, `neteisingas sudetingumas`, `per daug transakciju`, `neteisingas tx_root`, `neteisinga bloko maisa`, `netinkamas PoW`, `transakcija jau patvirtinta`, `transakcija bloke kartojasi`, `nepakankamas balansas`, `balanso perpildymas`.
Visos transakcijos taikomos kopijai. Tik jei visas blokas tinka, atnaujinami balansai, grandinė ir pool. Netinkamas blokas nieko nekeičia.

**Trūksta lėšų:** transakcija neatmetama, o lieka pool ir bandoma kituose blokuose (siuntėjas gali gauti pinigų). Kai nebelieka nė vienos įvykdomos, programa sustoja ir likusias parodo suvestinėje.

## Penki kandidatai

Kiekvienam blokui sudaromi 5 kandidatai. Kiekvienas atskirai sumaišo pool ir su savo balansų kopija atsirenka iki 100 transakcijų. Visi remiasi tuo pačiu `prev_hash` ir ta pačia patvirtinta būsena; kandidatų transakcijos gali persidengti.

- Kasama nuosekliai, kandidatai tikrinami tvarka #1 → #5.
- 1 raunde kiekvienas kandidatas gauna **5 000 bandymų**. Jei niekas nerado, biudžetas dvigubinamas (10 000, 20 000, ...), o paieška tęsiama nuo dar netikrinto `nonce`. Daugiausia 10 raundų, tada programa sustoja.
- **Laimėtojas:** iš radusių tame pačiame raunde – tas, kuriam prireikė mažiausiai bandymų (imituoja „kas pirmas rado“). Jei bandymų skaičius vienodas, laimi mažesnio numerio kandidatas.
- Laimėtojas dar kartą patikrinamas ir tik jo transakcijos pritaikomos. Kiti kandidatai išmetami ir balansų nepaveikia.

**Kodėl tai dar nėra P2P konsensusas:** viskas vyksta viename procese, kandidatai nesiunčia vienas kitam blokų, nėra tinklo vėlavimų, šakų (forks) ir jų sprendimo pagal ilgiausią grandinę. Visi kandidatai mato tą pačią būseną, o laimėtoją parenka viena programa pagal taisyklę – realiame tinkle kiekvienas mazgas sprendžia savarankiškai.

## Testai

`make test` – v0.2 turėjo 30 testų, v0.3 jų yra 38. Visi praeina.

| Scenarijus | Rezultatas |
|---|---|
| Galiojantis mokėjimas, blokas su <100 transakcijų | priimamas, balansai teisingi, suma nekinta |
| Nežinomas gavėjas, suma 0, pakeistas ID | atmetama su priežastimi |
| Du to paties siuntėjo mokėjimai viršija balansą (100: 70 + 60) | priimamas tik pirmas, blokas su abiem atmetamas |
| Dubliuotas ID bloke, jau patvirtinto mokėjimo kartojimas | atmetama |
| Pakeistas bloko turinys, `prev_hash`, netinkamas PoW, sumažintas sudėtingumas | blokas atmetamas, būsena nepasikeičia |
| Balanso perpildymas | atmetama |
| Merkle su 0, 1, 2, 3, 5 lapais, pakeistas lapas, sukeista tvarka | teisinga šaknis, pakeitus – kita šaknis |
| 5 kandidatai iš to paties pool | visi teisingi, balansai ir pool nepakinta |
| Ta pati transakcija / antraštė | tas pats ID / maiša |
| *v0.3:* nežinomas siuntėjas, siuntėjas = gavėjas | atmetama su priežastimi |
| *v0.3:* niekas negali sumokėti | kandidatas tuščias (ciklas sustoja) |
| *v0.3:* nepalaikoma versija, >100 transakcijų, pakeista bloko maiša | blokas atmetamas |
| *v0.3:* `mineWithBudget` maiša, kasimo tęsimas nuo neišbandyto `nonce` | maiša sutampa su `computeBlockHash`, randamas tas pats `nonce` |

Kolizijų tarp testuotų Merkle šaknų nerasta.

## Rezultatai (v0.2)

Bloko pavyzdys:

```
Blokas #1 | tx: 100 | nonce: 23 | bandymu: 24 | laikas: 7.62021 ms
  prev_hash: b6d67177dc21437aaa61a0d8a278ef878fd4cecf7a91d46060ddc61276a028de
  tx_root:   316bb114a8cde6804b60a8da735463cf2346344d2391ae686caa8549c5359d60
  hash:      000fb4dad5db317bb09865025245b7486832758f2c45e6fb3cf4fe5b55e49ff9 (difficulty 3)
  kandidatai: #1(100 tx, 3433, rado) #2(100 tx, 24, rado) #3(100 tx, 1477, rado) #4(100 tx, 476, rado) #5(100 tx, 3165, rado) -> laimejo #2, raundu: 1
```

`kandidatai` eilutė: kandidato numeris, transakcijų skaičius, atlikti bandymai ir ar rado tinkamą `nonce`.

Galutinė suvestinė:

```
Vartotoju: 1000
Sugeneruota transakciju: 10000
Priimta bloku (be genesis): 98
Itraukta transakciju: 9644
Atmesta pries pool: 0
Liko pool (nepakankamas balansas): 356
Pradine lesu suma: 481822578
Galutine lesu suma: 481822578
Bendras laikas: 1177.69 ms
Kandidatu bandymu is viso: 1364960
```

Lėšų suma pradžioje ir pabaigoje ta pati. Palyginti su v0.1, skaičiai šiek tiek skiriasi, nes kiekvienam blokui atsitiktinių skaičių generatorius naudojamas 5 kartus (penki kandidatai). Bendras laikas ilgesnis, nes kasami 5 kandidatai vietoj vieno.

## v0.3: patobulinimas su DI

Atskaita – žyma `v0.2`. Įrankis: **Claude (Anthropic), Claude Opus 5.5**, 2026-11-04. Kryptys: **kasimo ciklo efektyvumas** ir **testų aprėptis**. Kiekvienas priimtas pakeitimas – atskiras commit.

### Sąveikos žurnalas

**1. Kasimas.** Užklausa: *kaip pagreitinti kasimą, kai kiekvienam `nonce` iš naujo sujungiama visa antraštė; maišos funkcijos keisti negalima, maiša turi likti ta pati, testai turi praeiti* (pateiktos `serializeHeader`, `computeBlockHash`, `mineWithBudget`).

- ✅ **Priimta:** nekintamą antraštės dalį (`prev_hash|timestamp|version|tx_root|` ir `|difficulty`) sujungti vieną kartą prieš ciklą, cikle pridėti tik `nonce`. Maišai paduodami tie patys baitai.
- ❌ Pernaudoti eilutės buferį – efektas labai mažas, kodas sudėtingesnis.
- ❌ Optimizuoti maišos funkciją – keistų 1-ojo darbo maišą.
- ❌ Perkelti `nonce` į antraštės galą – pasikeistų formatas ir visos maišos.
- ❌ Lygiagretus kasimas – kitas sudėtingumo lygis, atskira papildoma užduotis.

**2. Testai.** Užklausa: *kokių svarbių atvejų 30 testų dar netikrina, nekeičiant programos logikos.*

- ✅ **Priimta (8 testai):** nežinomas siuntėjas, siuntėjas = gavėjas, tuščias kandidatas, nepalaikoma versija, >100 transakcijų, pakeistas `hash` laukas, `mineWithBudget` maiša = `computeBlockHash`, kasimo tęsimas nuo neišbandyto `nonce`.
- ❌ GoogleTest – išorinė priklausomybė, esamo `check` pakanka.
- ❌ Testas su visa 10 000 transakcijų grandine – lėtas ir nedeterministinis (`timestamp`).

**Kaip patikrinau kasimo pakeitimą:** `validateBlock` perskaičiuoja maišą senu būdu (`computeBlockHash`), todėl jei naujas kasimas duotų kitą maišą, testai „Netinkamas PoW atmetamas“, „Visi 5 kandidatai atskirai teisingi“ ir pilnas paleidimas blokus atmestų. Visi testai praėjo, blokų atmesta nebuvo, lėšų suma nepasikeitė. Greitis išmatuotas `make bench`.

### Palyginimas

Ta pati mašina (MacBook Air M4, Apple clang, `-O2`), ta pati maiša, `seed = 42`, sudėtingumas 3.

| | v0.2 | v0.3 |
|---|---|---|
| Testai (tas pats 38 testų rinkinys) | 38/38 | 38/38 |
| `make bench` vidurkis (200 000 bandymų) | 120,7 ms | **105,6 ms** |
| `make bench` min / max | 117,8 / 129,6 ms | 104,9 / 107,2 ms |
| Kasimo greitis | ~1,66 mln./s | **~1,89 mln./s** |
| Pilnas paleidimas: lėšų suma nekinta | ✅ | ✅ |

`make bench`: fiksuota antraštė, `difficulty = 64` (sąlyga neįvykdoma), todėl kiekvienas matavimas – lygiai 200 000 bandymų; 1 apšilimas + 5 matavimai, be konsolės išvesties. Pilno paleidimo laikas nelyginamas, nes dėl `timestamp` kiekvieną kartą skiriasi maišos, laimėtojai ir bandymų skaičius.

### Išvada

- **Pagerėjo:** kasimas ~12,5 % greitesnis (sklaidos intervalai nepersidengia); testų 30 → 38.
- **Nepasikeitė:** maiša, blokų formatas, validacija ir rezultatai. Pilno paleidimo laikas reikšmingai nesikeičia, nes didžiąją bandymo dalį užima pati maišos funkcija.
- **Liko apribojimai:** kandidatai kasami nuosekliai ir kopijuoja visą pool; maiša mokomoji, testai neįrodo saugumo.

## Šaltiniai

- Maišos funkcija – mano 1-ojo darbo kodas ([blockchain-tech-1](https://github.com/dziugasvas/blockchain-tech-1), v0.2).
- Kurso užduoties formuluotė ir joje nurodyti Bitcoin Developer Guide / Reference (blokų susiejimas, PoW, Merkle medis).
- Kitų žmonių kodo nenaudota; DI pagalba aprašyta skyriuose „v0.3“ ir „DI naudojimas“.

## DI naudojimas

v0.1 ir v0.2 kurtos su ChatGPT ir Claude (Anthropic) pagalba – struktūros planavimui, kodo generavimui, peržiūrai ir testams. v0.3 patobulinimai, jų vertinimas ir palyginimas aprašyti skyriuje „v0.3: patobulinimas su DI“.