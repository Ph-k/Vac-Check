# How to use the input file/directory creating scripts

As you can see there are two bash scripts:

**testFile.sh**: creates a file with random citizen names and ID’s of countries from a country file, with some random vaccination statuses for viruses from the virus file. 

Usage: `./testFile.sh virusesFile countriesFile numLines duplicatesAllowed` where:

- virusesFile: a file of virus names (one per line) (`viruses.txt` is an example of such a file)

- countriesFile: a file of country names (one per line) (`countries.txt` is an example of such a file)

- numLines: number of lines (aka vaccination records) that the resulting file will have.

- duplicatesAllowed: if set to 0, then the ID’s of the citizens will be unique. Otherwise duplicate citizen ID’s might result.

In any case the resulting inputFile should have the following format *(In case you want to create your own)*:

```
345 Maria Georgopoylos Greece 12 Η1Ν1 ΝΟ
674 Giorgos Soudas Greece 52 INFLUENZA YES 22-11-2021
943 Antonio Avantani Italy 54 COVID-19 YES 10-12-2020
231 Jack Gerald USA 81 H1N1 NO
```
**create_infiles.sh**: given an input file *(formatted as the file outputted from the testFile.sh)* it creates the file hierarchy in the correct format for the program.

Usage: `./create_infiles.sh inputFile input_dir numFilesPerDirectory` where:

- `inputFile`: is the above file. Created using the testFile sript, or any other way.

- `input_dir`: The name of the directory where the input file structure will be placed.

- `numFilesPerDirectory`: How many files will there be in each country directory.
