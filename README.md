# Seminární práce KIV/ZOS
## Pseudo-filesystém založený na i-uzlech

V této seminární práci byl implementován pseudo-filesystém založený na i-uzlech. Pseudo protože disk byl simulován souborem. 

### Filesystém podporuje:
* format <#MB>
* mkdir \<path>
* mv \<path1> \<path2>
* rm \<path>
* cp \<path1> \<path2>
* rmdir \<path>
* ls \<path>
* cat \<path>
* cd \<path>
* pwd \<path>
* info \<path> //Výsledek např. NAME – SIZE – i-node NUMBER – přímé a nepřímé odkazy
* incp \<path1> <path2>  //Nahraje soubor z pevného disku do pseudo-filesystému
* outcp \<path1> <path2> //Nahraje soubor z pseudo-filesystému na pevný disk
* load \<file> //Načte soubor s příkazy, které se sekvenčně vykonají



