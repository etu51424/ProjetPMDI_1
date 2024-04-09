//le code contient tte l'étape 1  + la partie complémentaire fichier validation aussi

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
#define NB_MOVEMENT_FOLDERS 15
#define NB_SUBJECTS 24
#define NB_MOVEMENTS_OF_PERSON NB_MOVEMENT_FOLDERS * NB_SUBJECTS //360

#define NB_MAX_LINES_READ_FOR_TRAINSET 600
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

typedef struct nbOutlierData NbOutlierData;
struct nbOutlierData {
	int X;
	int Y;
	int Z;
};

typedef struct outlierVAcc OutlierVAcc;
struct outlierVAcc {
	double X;
	double Y;
	double Z;
	int numLineX;
	int numLineY;
	int numLineZ;
};

//Détermine nbLines du fichier de Movement d'une personne (afin de savoir les 10% et 90%)
//lit le nombre affiché au début de la dernière ligne puis le retourne
int findNbLinesInFile(FILE* file) {
	char ch;
	long iCharFromTheEnd;

	// aller à la fin du fichier
	fseek(file, 0, SEEK_END);

	// but: atteindre début de la dernière ligne
	//-1 est le dernier caract du fichier
	iCharFromTheEnd = -1;
	do {
		// on va reculer d'un char
		iCharFromTheEnd--;
		// recule le curseur avec le i
		fseek(file, iCharFromTheEnd, SEEK_END);
		ch = fgetc(file);
	} while (ch != '\n'); // Stop quand atteint déb de la ligne

	// mtnt on est au deb de la dernière ligne
	//pour obtenir nbLinesDuFichier, lire ch par ch jusqu'à la prochaine virgule
	char strNbLinesInFile[T_MAX_INT_AS_STR];
	int iCharWord = 0;
	do {
		ch = fgetc(file);
		if (ch != ',' && !feof(file)) { //car on veut just concat les nombres pas les virgules
			strNbLinesInFile[iCharWord] = ch;
			iCharWord++;
		}
	} while (ch != ',' && !feof(file));

	//terminer la string
	strNbLinesInFile[iCharWord] = '\0';
	return atoi(strNbLinesInFile);
}

double findVectorMagnitude(double x, double y, double z) {
	return sqrt(x * x + y * y + z * z);
}

bool isOutlierData(double vAcc, double average, double stdDev) {
	return fabs(average - vAcc) > 3 * stdDev; //oui c'est >
}

// PRE CONDITION : ne peut etre appelé que dans une boucle qui lit un fichier du debut à la fin
// lire une ligne et la split en nb partitions demandées
// s'arrete a la fin d'une ligne (càd au premier \n)
// retenir le charactere où la fonction s'est arreté (numCharDeb) (175 pour commencer)
// reçoit donc le charactère depuis où il faut reprendre en paramètre
int partitionLineInDoubles(int nbPartitions, double partitions[], int numCharDeb, FILE* fiCsv) {

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
			if (ch != ',' && ch != '\n') { //car on veut just concat les nombres pas les virgules
				strncat_s(str, T_MAX_LINE, &ch, 1);
			}
			iChar++;
		} while (ch != ',' && ch != '\n' && !feof(fiCsv)); //feof utile pour data subject info
		partitions[iPartition] = strtod(str, &endptr); //str vers double

		iPartition++;
	} while (ch != '\n' && !feof(fiCsv));
	return iChar;
}

int writeOneLineToDataSetFile(OutlierVAcc outliersVAcc[600], NbOutlierData* pNbOutlierVAcc, int numCharDeb, int nbVAccColumns, FILE* inputFile, FILE* outputFile, int iMovement, int gender, int index) {
	int movement;
	double partitions[NB_PARTITIONS]; //a 13 colonnes comme tout movementOfSubject file
	double vAcc;
	int lastCharPos = numCharDeb;

	// dans l'énoncé, des numéros de 1-6 sont associés à chaque type de mouvement (wlk, downwstairs, etc). 
	// ce switch prend l'indice du fichier et regroupe les noms au nombre associé au type de mouvement comme l'énoncé le requiert
	switch (iMovement) {
	case 0:case 1:case 2:
		movement = 1;
		break;
	case 3:case 4:
		movement = 2;
		break;
	case 5:case 6:
		movement = 4;
		break;
	case 7:case 8:
		movement = 5;
		break;
	case 9:case 10:case 11:
		movement = 3;
		break;
	case 12:case 13:case 14:
		movement = 6;
		break;
	};
	fprintf_s(outputFile, "%d", movement);
	fputs(",", outputFile);
	fprintf_s(outputFile, "%d", gender);
	fputs(",", outputFile);
	fprintf_s(outputFile, "%d", index);
	fputs(",", outputFile);

	for (int iLine = 0; iLine < nbVAccColumns; iLine++) {
		// 13 colonnes dans la ligne : tps|x|y|z|x|y|z|x|y|z|x|y|z|
		lastCharPos = partitionLineInDoubles(NB_PARTITIONS, partitions, lastCharPos, inputFile);
		vAcc = findVectorMagnitude(partitions[10], partitions[11], partitions[12]);
		fprintf(outputFile, "%lf", vAcc);

		if (iLine == nbVAccColumns - 1) {
			fputs("\n", outputFile);
		} else {
			fputs(",", outputFile);
		}

		//PHASE 1.5
		if (isOutlierData(partitions[10], AVERAGE_X, STD_DEV_X)) {
			
			//printf(DEEPPINK"%lf"RESET, partitions[10]);
			//printf(RED"%d"RESET, pNbOutlierVAcc->X);
			outliersVAcc[pNbOutlierVAcc->X].X = partitions[10];
			outliersVAcc[pNbOutlierVAcc->X].numLineX = iLine;
			pNbOutlierVAcc->X++;
		}

		if (isOutlierData(partitions[11], AVERAGE_Y, STD_DEV_Y)) {
			pNbOutlierVAcc->Y++;
			//printf(AQUAMARINE3"%lf"RESET, partitions[11]);
			outliersVAcc[pNbOutlierVAcc->Y].Y = partitions[11];
			outliersVAcc[pNbOutlierVAcc->Y].numLineY = iLine;
			pNbOutlierVAcc->Y++;
		}

		if (isOutlierData(partitions[12], AVERAGE_Z, STD_DEV_Z)) {
			//printf(CYAN"%lf"RESET, partitions[12]);
			outliersVAcc[pNbOutlierVAcc->Z].Z = partitions[12];
			outliersVAcc[pNbOutlierVAcc->Z].numLineZ = iLine;
			pNbOutlierVAcc->Z++;
		}
	}
	return lastCharPos;
}

void createTestSetAndTrainSet(void) {

	FILE* fiSubjectsInfo;
	FILE* fiTrainSet;
	FILE* fiTestSet;
	FILE* fiValidation;

	int lastCharPosSubjectInfo = 34;
	char movementFolders[NB_MOVEMENT_FOLDERS][T_FILEPATH] = { "dws_1", "dws_2", "dws_11", "jog_9", "jog_16", "sit_5",
	"sit_13", "std_6", "std_14", "ups_3", "ups_4","ups_12", "wlk_7", "wlk_8", "wlk_15" };
	char filepath[T_FILEPATH];

	{
		fopen_s(&fiTrainSet, "../TrainSet.csv", "w");
		if (fiTrainSet == NULL) {
			fprintf(stderr, "Erreur ouverture fi Trainset\n");
			exit(EXIT_FAILURE);
		}

		fopen_s(&fiTestSet, "../TestSet.csv", "w");
		if (fiTestSet == NULL) {
			fprintf(stderr, "Erreur ouverture fi TestSet\n");
			exit(EXIT_FAILURE);
		}

		fopen_s(&fiSubjectsInfo, "../data_subjects_info.csv", "r");
		if (fiSubjectsInfo == NULL) {
			fprintf(stderr, "Erreur ouverture fichier data subject infos \n");
			exit(EXIT_FAILURE);
		}

		fopen_s(&fiValidation, "../Validation.csv", "w");
		if (fiValidation == NULL) {
			fprintf(stderr, "Erreur ouverture fi Validation\n");
			exit(EXIT_FAILURE);
		}
	}

	//fichiers commencent à sub_1
	
	for (int iMovement = 0; iMovement < NB_MOVEMENT_FOLDERS; iMovement++) {
		
		for (int iPerson = 1; iPerson <= NB_SUBJECTS; iPerson++) {
			double subjectInfo[5];
			FILE* fiMovementOfPerson;
			char strPerson[T_MAX_INT_AS_STR];
			int nbLinesInFile;
			int lastCharPos;
			OutlierVAcc outliersVAcc[NB_MAX_LINES_READ_FOR_TRAINSET];
			NbOutlierData nbOutlierVAcc = { 0 };  // initx,y,z à 0 pour chaque fichier mouvement

			// réouvre le fichier pour chaque personne
			lastCharPosSubjectInfo = partitionLineInDoubles(5, subjectInfo, lastCharPosSubjectInfo, fiSubjectsInfo);
			lastCharPosSubjectInfo++; //rustine sinon reste une fois de trop sur la meme ligne de fiSUbjectInfo

			sprintf_s(strPerson, T_MAX_INT_AS_STR, "%d", iPerson);
			strcpy_s(filepath, T_FILEPATH, "../A_DeviceMotion_data/A_deviceMotion_data/");
			strcat_s(filepath, T_FILEPATH, movementFolders[iMovement]);
			strcat_s(filepath, T_FILEPATH, "/sub_");
			strcat_s(filepath, T_FILEPATH, strPerson);
			strcat_s(filepath, T_FILEPATH, ".csv");
			puts(filepath);

			fopen_s(&fiMovementOfPerson, filepath, "r");
			if (fiMovementOfPerson == NULL) {

				fprintf(stderr, "Erreur ouverture fiMovementOfPerson\n");
				exit(EXIT_FAILURE);
			}

			nbLinesInFile = findNbLinesInFile(fiMovementOfPerson);
			printf("%d\n", nbLinesInFile);
			// lis 600 lignes vers le dataset et 60 lignes vers le testset.
			// Si le fichier a moins de 660 lignes, alors 90% du fichier va vers le dataset et 10% vers le testset
			if (nbLinesInFile < NB_MAX_LINES_READ_FOR_ALL_DATASETS) {
				// TRAIN SET
				lastCharPos = writeOneLineToDataSetFile(outliersVAcc, &nbOutlierVAcc, START_OF_DATASET, (int)(0.9 * nbLinesInFile), fiMovementOfPerson, fiTrainSet, iMovement, (int)subjectInfo[4], (int)subjectInfo[0]);
				// TEST SET
				//attention, doit commencer à lire à partir de la ligne 0.9 * nbLinesInFile (càd 600 pour la plupart)
				lastCharPos = writeOneLineToDataSetFile(outliersVAcc, &nbOutlierVAcc, lastCharPos, (int)(0.1 * nbLinesInFile), fiMovementOfPerson, fiTestSet, iMovement, (int)subjectInfo[4], (int)subjectInfo[0]);
			} else {
				//TRAIN SET
				lastCharPos = writeOneLineToDataSetFile(outliersVAcc, &nbOutlierVAcc, START_OF_DATASET, NB_MAX_LINES_READ_FOR_TRAINSET, fiMovementOfPerson, fiTrainSet, iMovement, (int)subjectInfo[4], (int)subjectInfo[0]);
				//TEST SET
				//attention, doit commencer à 600
				lastCharPos = writeOneLineToDataSetFile(outliersVAcc, &nbOutlierVAcc, lastCharPos, NB_MAX_LINES_READ_FOR_TESTSET, fiMovementOfPerson, fiTestSet, iMovement, (int)subjectInfo[4], (int)subjectInfo[0]);
			}

			fputs(filepath, fiValidation);
			fputs(",", fiValidation);
			fprintf(fiValidation, "%d", nbLinesInFile);
			fputs(",", fiValidation);
			fprintf(fiValidation, "%d", nbOutlierVAcc.X);
			fputs(",", fiValidation);
			fprintf(fiValidation, "%d", nbOutlierVAcc.Y);
			fputs(",", fiValidation);
			fprintf(fiValidation, "%d", nbOutlierVAcc.Z);
			fputs("\n", fiValidation);

			printf(AQUAMARINE3"|%d}"RESET, nbOutlierVAcc.X);

			// écrit nbDonnesAbberantes X, Y et Z
			// écrit 3 lignes de 1200 colonnes
			for (int iOutlierVAcc = 0; iOutlierVAcc < nbOutlierVAcc.X; iOutlierVAcc++) {
				printf(LIGHTSTEELBLUE3"%d"RESET, outliersVAcc[iOutlierVAcc].numLineX);
				fprintf(fiValidation, "%d", outliersVAcc[iOutlierVAcc].numLineX);
				fputs(",", fiValidation);
				fprintf(fiValidation, "%lf", outliersVAcc[iOutlierVAcc].X);
				printf(GREEN"%lf"RESET, outliersVAcc[iOutlierVAcc].X);
				fputs(",", fiValidation);
			}
			fputs("\n", fiValidation);

			for (int iOutlierVAcc = 0; iOutlierVAcc < nbOutlierVAcc.Y; iOutlierVAcc++) {
				fprintf(fiValidation, "%d", outliersVAcc[iOutlierVAcc].numLineY);
				fputs(",", fiValidation);
				fprintf(fiValidation, "%lf", outliersVAcc[iOutlierVAcc].Y);
				fputs(",", fiValidation);
			}
			fputs("\n", fiValidation);

			for (int iOutlierVAcc = 0; iOutlierVAcc < nbOutlierVAcc.Z; iOutlierVAcc++) {
				fprintf(fiValidation, "%d", outliersVAcc[iOutlierVAcc].numLineZ);
				fputs(",", fiValidation);
				fprintf(fiValidation, "%lf", outliersVAcc[iOutlierVAcc].Z);
				fputs(",", fiValidation);
			}
			fputs("\n", fiValidation);

			fclose(fiMovementOfPerson);
		}
	}
	fclose(fiSubjectsInfo);
	fclose(fiTestSet);
	fclose(fiTrainSet);
	fclose(fiValidation);
}

//les fonctions ci-dessous sont ignorées dans le Diagramme d'action, ce sont juste des menus et de l'affichage
void getNbColumnsOfDataSetLine(int nbColumnsOfDataSetLine[NB_MOVEMENTS_OF_PERSON], bool isTestSet) {
	FILE* fiValidation;

	fopen_s(&fiValidation, "../Validation.csv", "r");
	if (fiValidation == NULL) {

		fprintf(stderr, "Erreur ouverture fi Validation\n");
		exit(EXIT_FAILURE);
	}

	for (int iLine = 0; iLine < NB_MOVEMENTS_OF_PERSON; iLine++) {
		int iPartition = 0;
		char str[T_MAX_LINE];
		char ch;
		int nbLinesInThatFile;

		do {
			str[0] = '\0'; //vider la string!
			do {
				ch = fgetc(fiValidation);

				if (ch != ',' && ch != '\n') { //car on veut just concat les nombres pas les virgules
					strncat_s(str, T_MAX_LINE, &ch, 1);
				}
			} while (ch != ',' && ch != '\n');

			if (iPartition == 1) {
				nbLinesInThatFile = atoi(str);

				if (nbLinesInThatFile < NB_MAX_LINES_READ_FOR_TRAINSET) {
					if (isTestSet) {
						nbColumnsOfDataSetLine[iLine] = 0.1 * nbLinesInThatFile + 3;
					} else {
						nbColumnsOfDataSetLine[iLine] = nbLinesInThatFile + 3;
					}
				} else {
					if (isTestSet) {
						nbColumnsOfDataSetLine[iLine] = NB_MAX_LINES_READ_FOR_TESTSET +3; //tjr 63
					} else {
						nbColumnsOfDataSetLine[iLine] = NB_MAX_DATASET_COLUMNS; //603
					}
				}
			}
			iPartition++;
		} while (ch != '\n' && !feof(fiValidation));
	}
	fclose(fiValidation);
}

void displayDataSet(char filename[T_FILEPATH], bool isTestSet) {
	FILE* fiDataSet;
	int nbColumnsForLineInDataset[NB_MOVEMENTS_OF_PERSON];

	fopen_s(&fiDataSet, filename, "r");
	if (fiDataSet == NULL) {

		fprintf(stderr, "Erreur ouverture %s\n", filename);
		exit(EXIT_FAILURE);
	}
	//ici faut une fn qui connait nbLines du fichier pour pouvoir connaitre le nombre de partitions à faire. Cette info se trouve dans le fiValidation
	getNbColumnsOfDataSetLine(nbColumnsForLineInDataset, isTestSet);

	int lastCharPos = 0; //et non 175
	for (int iLine = 0; iLine < NB_MOVEMENTS_OF_PERSON; iLine++) {
		double partitions[NB_MAX_DATASET_COLUMNS];

		lastCharPos = partitionLineInDoubles(nbColumnsForLineInDataset[iLine], partitions, lastCharPos, fiDataSet);
		lastCharPos++; //rustine car la fn prec termine un char trop tôt

		printf(YELLOW"%de ligne\n"RESET, iLine+1);
		for (int iPartition = 0; iPartition < nbColumnsForLineInDataset[iLine]; iPartition++) {
			printf(RED" %3d:"RESET, iPartition+1);

			switch (iPartition) {
			case 0:
				printf(ORANGE"%8lf "RESET, partitions[iPartition]);
				break;
			case 1:
				printf(AQUAMARINE3"%8lf "RESET, partitions[iPartition]);
				break;
			case 2:
				printf(PLUM1"%8lf "RESET, partitions[iPartition]);
				break;
			default:
				printf("%8lf ", partitions[iPartition]);
				break;
			};
		}
		putchar('\n');
	}
	fclose(fiDataSet);
}

int choixLu() {
	int choix;
	printf("1-create Testset and Trainset\n2-display train set\n3-display test set\n5-quitter"
		"\nChoix : ");
	scanf_s("%d", &choix);
	getchar();
	return choix;
}

void main(void) {
	int choix;

	choix = choixLu();
	do {
		switch (choix) {
		case 1:
			createTestSetAndTrainSet();
			break;
		case 2:
			displayDataSet("../TrainSet.csv", false);
			break;
		case 3:
			displayDataSet("../TestSet.csv", true);
			break;
		}
		choix = choixLu();
	} while (choix != 5);
}