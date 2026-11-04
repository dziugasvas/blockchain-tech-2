# Supaprastinta blokų grandinė

Blokų grandinių technologijų 2-oji užduotis. C++ programa, kuri sukuria vartotojus ir transakcijas, sudeda jas į blokus, iškasa blokus (Proof-of-Work), patikrina juos ir atnaujina balansus.

## v0.1

- 1 000 vartotojų, 10 000 transakcijų, blokas iki 100 transakcijų
- `seed = 42` (`std::mt19937`)
- PoW sudėtingumas 3: bloko maiša turi prasidėti `000`
- Sąskaitų modelis: `public_key -> balansas`, be komisinių ir kasimo atlygio
- Pradinė būsena įrašoma į `initial_state.csv` prieš pritaikant transakcijas

## Paleidimas

Reikia `g++` su C++17 (testuota macOS, Apple clang, MacBook Air M4).

```bash
make
./blockchain          # visas rinkinys
./blockchain small    # mažas pavyzdys: 5 vartotojai, 20 transakcijų
```

## Maiša

Naudoju savo 1-ojo darbo maišos funkciją – [blockchain-tech-1](https://github.com/dziugasvas/blockchain-tech-1), versija **v0.2** (256 bitai, 64 hex simboliai). Rinkausi v0.2, nes ankstesnėje versijoje buvo lengvai randamų kolizijų.

| Kas | Kaip skaičiuojama |
|---|---|
| `public_key` | `H(vardas)`, pvz. `H("User1")` |
| transakcijos ID | `H("sender\|receiver\|amount\|sequence")` |
| `tx_root` | `H(visi bloko transakcijų ID iš eilės)` |
| bloko maiša | `H("prev_hash\|timestamp\|version\|tx_root\|nonce\|difficulty")` |

Laukai atskiriami `|`, kad skirtingi duomenys nesusijungtų į tą pačią eilutę (`1|23` ≠ `12|3`).

## Modelis

- **Vartotojas:** vardas (`User1`...), `public_key`, balansas nuo 100 iki 1 000 000.
- **Transakcija:** siuntėjas, gavėjas (skirtingi), suma nuo 1 iki 100 000, unikalus `sequence`, ID.
- **Blokas:** antraštė iš 6 laukų ir transakcijų sąrašas.
- **Genesis:** `prev_hash` – 64 nuliai, `timestamp = 0`, `tx_root = H("")`, nekasamas (`difficulty = 0`).

## Kaip veikia ciklas

1. Į pool patenka tik transakcijos su žinomais dalyviais, teigiama suma ir teisingu ID.
2. Pool sumaišomas, imamos transakcijos, kurias siuntėjas gali apmokėti. Tai tikrinama su balansų kopija, todėl du to paties žmogaus mokėjimai negali kartu viršyti jo balanso.
3. Blokas iškasamas – `nonce` didinamas, kol maiša prasideda `000`.
4. Prieš pridedant blokas patikrinamas dar kartą: `prev_hash`, `tx_root`, perskaičiuota maiša ir visos transakcijos. Tik tada keičiami balansai ir transakcijos išimamos iš pool.
5. Jei siuntėjui trūksta pinigų, transakcija lieka pool ir bandoma vėliau. Kai nebelieka nė vienos įvykdomos, programa sustoja.

## Rezultatai

Bloko pavyzdys (paskutinis, nepilnas blokas):

```
Blokas #98 | tx: 3 | nonce: 7231 | bandymu: 7232 | laikas: 4.01867 ms
  prev_hash: 0002c32c68e139b7e014e7616c2d359aceaf0f8a306720b86bc550054eec36ad
  tx_root:   383f55c3b8fa860f0d514f2f2d0502111e142a6643884452ced900f39b3983f6
  hash:      000e1842dbd9bc1bb6efdd65dbfd7c7534862c3b15dc7b588c82df1d2a819ff0 (difficulty 3)
```

Galutinė suvestinė:

```
Vartotoju: 1000
Sugeneruota transakciju: 10000
Priimta bloku (be genesis): 98
Itraukta transakciju: 9660
Atmesta pries pool: 0
Liko pool (nepakankamas balansas): 340
Pradine lesu suma: 481822578
Galutine lesu suma: 481822578
Bendras laikas: 345.907 ms
```

Įtrauktos 9 660 transakcijų, likusios 340 neįvykdytos, nes siuntėjams trūko lėšų. Lėšų suma pradžioje ir pabaigoje ta pati, vadinasi, pinigai neatsirado ir nedingo.

## DI naudojimas

v0.1 kurta su ChatGPT ir Claude (Anthropic) pagalba – struktūros planavimui ir kodo peržiūrėjimui.