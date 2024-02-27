
// Definizione della struttura per i punti
typedef struct {
  double x;
  double y;
} Point;

// Definizione della struttura per i centroidi
typedef struct {
  Point point;
} Centroid;

#define MAX 100

//Struttura dentro al messaggio
typedef struct {
  double variance;
  Centroid centroids[MAX];
} Msg;

// Definizione della struttura per il messaggio
typedef struct {
  long mtype;
  Msg msg;
} Message;

