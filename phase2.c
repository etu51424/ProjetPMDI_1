//PHASE2 data

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>

#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"
#define YELLOW  "\x1b[33m"
#define BLUE    "\x1b[34m"
#define MAGENTA "\x1b[35m"
#define CYAN    "\x1b[36m"

#define ORANGE "\033[38;5;208m"
#define DARKGOLDENROD "\033[38;5;136m"
#define GOLD3 "\033[38;5;142m"
#define LIGHTSALMON3 "\033[38;5;138m"
#define DEEPPINK "\033[38;5;199m"
#define GREY46 "\033[38;5;243m"
#define AQUAMARINE3 "\033[38;5;79m"
#define MAROON "\033[38;5;1m"
#define DARKSEAGREEN "\033[38;5;65m"
#define BROWN "\033[38;5;94m"
#define YELLOWORANGE "\033[38;5;214m"
#define PLUM1 "\033[38;5;219m"
#define LIGHTSTEELBLUE3 "\033[38;5;146m"

#define RESET   "\x1b[0m"

#define START_OF_DATASET 175
#define T_MAX_DOUBLE_AS_STR 70 //arbitraire
#define T_MAX_INT_AS_STR 20
#define T_MAX_LINE 1000

#define NB_PARTITIONS 13
#define NB_COLUMNS_VALIDATION_FILE 2

#define T_FILEPATH 200
#define NB_MOVEMENTS 15
#define NB_SUBJECTS 24
#define NB_MOVEMENTS_OF_PERSON NB_MOVEMENTS * NB_SUBJECTS //360
#define NB_MAX_COLUMNS_IN_TRAINSET 600 
#define NB_MAX_LINES_READ_FOR_TESTSET 60

#define NB_MAX_DATASET_COLUMNS NB_MAX_LINES_READ_FOR_TRAINSET + 3 //+ movement + gender + index
#define NB_MAX_LINES_READ_FOR_ALL_DATASETS 660

#define AVERAGE_X 0.00096087
#define STD_DEV_X 0.38875666
#define AVERAGE_Y 0.05525659
#define STD_DEV_Y 0.61937128
#define AVERAGE_Z 0.0352192
#define STD_DEV_Z 0.4300345

//N'ouvrez aucun des fichiers utilisés ici lors de la compilation sinon erreurs

// On nous a dit que vous aviez dit que 600 lignes était suffisant
//s'il faut effectivement 10% et 90% de chaque fichier (et donc + de 660 lignes),
// je peux modifier mon code aisément (j'ai déjà la fonction qui calcule le nombre de lignes)


//change le iLine enun num de char car dans la fn readLine c'est char par char avec fseek
int partitionLineInDoubles(double partitions[], int numCharDeb, FILE* fiCsv, int* pNbPartition) {

	fseek(fiCsv, numCharDeb, SEEK_SET);
	char str[T_MAX_LINE];
	int iPartition = 0;
	char ch;
	char* endptr;
	int iChar = numCharDeb;

	do {
		str[0] = '\0'; //vider la string!
		do {
			ch = fgetc(fiCsv);
			//putchar(ch);
			if (ch != ',' && ch != '\n') { //car on veut just concat les nombres pas les virgules
				strncat_s(str, T_MAX_LINE, &ch, 1);
			}
			iChar++;
		} while (ch != ',' && ch != '\n' && !feof(fiCsv)); //feof utile pour data subject info
		partitions[iPartition] = strtod(str, &endptr); //str vers double

		iPartition++;
	} while (ch != '\n' && !feof(fiCsv));
	
	*pNbPartition = iPartition;
	return iChar+1; //+1 pour passer le caract \n
}


void main(void) {
	FILE* fiTrainSet;
	FILE* fiPatterns;
	double sumsOfVAcc[600];
	int nbVAccsOfMovementColumn[600];
	double partitions[603];

	{
		fopen_s(&fiTrainSet, "../TrainSet.csv", "r");
		if (fiTrainSet == NULL) {
			fprintf(stderr, "Erreur ouverture fi Trainset\n");
			exit(EXIT_FAILURE);
		}
		fopen_s(&fiPatterns, "../patterns.csv", "w");
		if (fiPatterns == NULL) {
			fprintf(stderr, "Erreur ouverture fi Patterns\n");
			exit(EXIT_FAILURE);
		}
	}

	for (int iColumn = 0; iColumn < 600; iColumn++) {
		sumsOfVAcc[iColumn] = 0;
		nbVAccsOfMovementColumn[iColumn] = 0; //pour etre sur
	}
	int lastCharPos = 0;

	for (int iMovement = 0; iMovement < 6; iMovement++) { // 0 1 2 3 4 5
		
		int nbPartitions = 0; //l'init à qqch pour pouvoir etre passé en param
		partitionLineInDoubles(partitions, lastCharPos, fiTrainSet, &nbPartitions); //lire sans rien renvoyer, juste pour connaitre partitions[0] avant la boucle
		int currentMovementNum = (int)partitions[0];
		int nbLigesOfMovement = 0;

		while (iMovement < 6 && currentMovementNum == partitions[0]) {
			lastCharPos = partitionLineInDoubles(partitions, lastCharPos, fiTrainSet, &nbPartitions);

			for (int i = 0; i < 6; i++) printf(RED"%lf"RESET, partitions[i]);
			printf(DEEPPINK"%d"RESET, iMovement);
			printf(CYAN"%d"RESET, lastCharPos);
			printf(GREEN"%d "RESET "==" ORANGE"%lf"RESET " donne "MAGENTA"%d"RESET "\n", currentMovementNum, partitions[0], currentMovementNum == partitions[0]);

			// si nbPartitions < 603, dans les 2 tableaux, les cases entre nbPartitions et 603 seront ignorees lors de la moyenne
			for (int iPartition = 3; iPartition < nbPartitions; iPartition++) {

				sumsOfVAcc[iPartition - 3] += partitions[iPartition];
				nbVAccsOfMovementColumn[iPartition - 3]++;
			}
			nbLigesOfMovement++;
		} 

		fprintf(fiPatterns, "%d", iMovement);
		fputs(",", fiPatterns);

		// pour toutes les colonnes du movement donc nb colonnes à parcourir sera tjr 600
		// sumVAccs[599] et nbVAccsOfMovementColumn[599]
		// valent au pire 0 (mais ça n'arrivera pas)
		for (int iVAcc = 0; iVAcc < 600; iVAcc++) {
			//printf(RED"%lf"RESET "/" BLUE"%d"RESET, sumsOfVAcc[iVAcc], nbVAccsOfMovementColumn[iVAcc]);
			double averageVAcc = sumsOfVAcc[iVAcc] / nbVAccsOfMovementColumn[iVAcc];
			
			fprintf(fiPatterns, "%lf", averageVAcc);
			fputs(",", fiPatterns);
		}

		fputs("\n", fiPatterns);
	}
	/*
	int iLine = 0;
	for (int iMovement = 0; iMovement < 6; iMovement++) {
		double partitions[NB_MAX_COLUMNS_IN_TRAINSET]; 
		// determiner ligne de debut et de fin pour le mouvement (pour pas le relire à chaq col)

		int numLineMovementStart = iLine;
		readLine(partitions, fiTrainSet, 0);
		int numCurrentMovement = partitions[0];
		fprintf(fiPatterns, "%d", numCurrentMovement);

		while (partitions[0] == numCurrentMovement) {
			partitions[0] = 0;
			iLine++;
			readLine(partitions, fiTrainSet, iLine);
		}
		int numLineMovementEnd = iLine;

		// 600 marchera pas tt de suite car pas tous ont 600 columns
		int iLinePerson = 0;
		for (int iColumnVAcc = 0; iColumnVAcc < NB_MAX_COLUMNS_IN_TRAINSET; iColumnVAcc++) {
			int sumVAccs = 0;

			for (iLinePerson = numLineMovementStart; iLinePerson < numLineMovementEnd; iLinePerson++) {
				readLine(partitions, fiTrainSet, iLine);
				sumVAccs += partitions[iColumnVAcc];
			}

			double averageVAcc = sumVAccs / (numLineMovementEnd - numLineMovementStart);
			fprintf(fiPatterns, "%lf", averageVAcc);

			if (iColumnVAcc = NB_MAX_COLUMNS_IN_TRAINSET - 1) {
				fputs("\n", fiPatterns);
			} else {
				fputs(fiPatterns, ",");
			}
		}
	}*/

	fclose(fiTrainSet);
	fclose(fiPatterns);
}

