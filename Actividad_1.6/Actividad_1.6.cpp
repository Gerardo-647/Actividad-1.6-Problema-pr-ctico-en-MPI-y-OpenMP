/* Integrantes (Orden Alfabético):
 * - Cancelada de la O Gerardo Alexander
 * - Hernández Miranda David Alejandro
 */

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <string>
#include <mpi.h>
#include <omp.h>

using namespace std;

void imprimirIntegrantes() {
    cout << "-----------------------------------------------------------------\n";
    cout << " Integrantes del Equipo:\n";
    cout << " 1. Cancelada de la O, Gerardo Alexander\n";
    cout << " 2. Hernandez Miranda, David Alejandro\n";
    cout << "-----------------------------------------------------------------\n";
}

const char CHARSET[36] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
    'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
    'U', 'V', 'W', 'X', 'Y', 'Z', '0', '1', '2', '3',
    '4', '5', '6', '7', '8', '9'
};

void convertirIndiceATexto(unsigned long long idx, char* candidato, int longitud) {
    for (int i = longitud - 1; i >= 0; i--) {
        candidato[i] = CHARSET[idx % 36];
        idx /= 36;
    }
    candidato[longitud] = '\0';
}

bool validarClave(const char* clave, int longitud) {
    for (int i = 0; i < longitud; i++) {
        bool valido = false;
        for (int c = 0; c < 36; c++) {
            if (clave[i] == CHARSET[c]) {
                valido = true;
                break;
            }
        }
        if (!valido) return false;
    }
    return true;
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    char hostname[MPI_MAX_PROCESSOR_NAME];
    int hostname_len;
    MPI_Get_processor_name(hostname, &hostname_len);
    string pc_name(hostname);

    if (rank == 0) {
        imprimirIntegrantes();
    }

    int opcion = 0;
    int longitud_clave = 0;
    char* clave_objetivo = nullptr;

    double tiempo_secuencial = 0.0;
    double tiempo_paralelo = 0.0;
    bool clave_ingresada = false;

    do {
        if (rank == 0) {
            cout << "\n===== MENU INTERACTIVO - FUERZA BRUTA DISTRIBUIDA (MPI + OpenMP) =====\n";
            cout << "1. Ingresar clave personalizada en mayusculas (A-Z, 0-9)\n";
            cout << "2. Ejecutar busqueda en modo Secuencial (1 nodo, 1 hilo - Linea Base)\n";
            cout << "3. Ejecutar busqueda en modo Paralelo Distribuido (MPI + OpenMP)\n";
            cout << "4. Mostrar tabla comparativa de tiempos y ganancia de velocidad (Speedup)\n";
            cout << "5. Salir\n";
            cout << "Seleccione una opcion: ";
            cin >> opcion;
        }

        MPI_Bcast(&opcion, 1, MPI_INT, 0, MPI_COMM_WORLD);

        if (opcion == 5) {
            string log_filename = "log_equipo_" + pc_name + "_nodo_" + to_string(rank) + ".txt";
            ofstream log_file(log_filename, ios::out | ios::app);
            log_file << "[Finalizado] Salida limpia del programa." << endl;
            log_file.close();
            break;
        }

        if (opcion == 1) {
            if (rank == 0) {
                string input_clave;
                cout << "Ingrese la clave alfanumerica a descifrar (ej. X7B): ";
                cin >> input_clave;
                longitud_clave = input_clave.length();

                if (longitud_clave > 10 || !validarClave(input_clave.c_str(), longitud_clave)) {
                    cout << "Error: Clave invalida o demasiado larga (max 10 caracteres).\n";
                    longitud_clave = 0;
                    clave_ingresada = false;
                }
                else {
                    clave_objetivo = new char[longitud_clave + 1];
                    strcpy_s(clave_objetivo, longitud_clave + 1, input_clave.c_str());
                    clave_ingresada = true;
                    cout << "Clave registrada correctamente. Longitud: " << longitud_clave << "\n";
                }
            }

            MPI_Bcast(&clave_ingresada, 1, MPI_C_BOOL, 0, MPI_COMM_WORLD);
            if (!clave_ingresada) continue;

            MPI_Bcast(&longitud_clave, 1, MPI_INT, 0, MPI_COMM_WORLD);
            if (rank != 0) {
                clave_objetivo = new char[longitud_clave + 1];
            }
            MPI_Bcast(clave_objetivo, longitud_clave + 1, MPI_CHAR, 0, MPI_COMM_WORLD);

        }
        else if (opcion == 2) {
            if (!clave_ingresada) {
                if (rank == 0) cout << "[Aviso] Debe ingresar una clave primero (Opcion 1).\n";
                continue;
            }

            unsigned long long total_combinaciones = static_cast<unsigned long long>(pow(36.0, longitud_clave));

            if (rank == 0) {
                cout << "\n[Ejecucion Secuencial] Iniciando busqueda con 1 hilo en Nodo 0...\n";
                double t_inicio = MPI_Wtime();
                bool encontrado_seq = false;
                char candidato_seq[11];

                for (unsigned long long idx = 0; idx < total_combinaciones; idx++) {
                    convertirIndiceATexto(idx, candidato_seq, longitud_clave);
                    if (strcmp(candidato_seq, clave_objetivo) == 0) {
                        encontrado_seq = true;
                        break;
                    }
                }

                double t_fin = MPI_Wtime();
                tiempo_secuencial = t_fin - t_inicio;
                if (tiempo_secuencial == 0.0) tiempo_secuencial = 1e-6;

                cout << "[Resultado Secuencial] Clave encontrada en " << tiempo_secuencial << " segundos.\n";

                string log_filename = "log_equipo_" + pc_name + "_nodo_" + to_string(rank) + ".txt";
                ofstream log_file(log_filename, ios::out | ios::app);
                log_file << "[SECUENCIAL] Clave: " << clave_objetivo << " | Tiempo total: " << tiempo_secuencial << "s" << endl;
                log_file.close();
            }

        }
        else if (opcion == 3) {
            if (!clave_ingresada) {
                if (rank == 0) cout << "[Aviso] Debe ingresar una clave primero (Opcion 1).\n";
                continue;
            }

            string log_filename = "log_equipo_" + pc_name + "_nodo_" + to_string(rank) + ".txt";
            ofstream log_file(log_filename, ios::out | ios::trunc);

            unsigned long long total_combinaciones = static_cast<unsigned long long>(pow(36.0, longitud_clave));
            int num_esclavos = size - 1;
            if (num_esclavos < 1) num_esclavos = 1;

            unsigned long long chunk = total_combinaciones / num_esclavos;
            unsigned long long inicio_local = 0;
            unsigned long long fin_local = 0;

            if (rank != 0) {
                int esclavo_id = rank - 1;
                inicio_local = esclavo_id * chunk;
                fin_local = (esclavo_id == num_esclavos - 1) ? total_combinaciones - 1 : inicio_local + chunk - 1;

                log_file << "[Equipo: " << pc_name << "] [Nodo MPI: " << rank
                    << "] [Rango Asignado: " << inicio_local << " - " << fin_local << "] [Estado: Iniciado]" << endl;
            }
            else {
                log_file << "[Equipo: " << pc_name << "] [Nodo Maestro] [Iniciando coordinacion paralela]" << endl;
            }

            double tiempo_inicio = MPI_Wtime();
            bool encontrado_global = false;
            char clave_encontrada[11];
            unsigned long long indice_encontrado = 0;
            int nodo_ganador = -1;
            int hilo_ganador = -1;

            if (rank == 0) {
                int flag = 0;
                MPI_Status status;
                unsigned long long mensaje_exito[3];

                while (!encontrado_global) {
                    MPI_Iprobe(MPI_ANY_SOURCE, 100, MPI_COMM_WORLD, &flag, &status);
                    if (flag) {
                        MPI_Recv(mensaje_exito, 3, MPI_UNSIGNED_LONG_LONG, status.MPI_SOURCE, 100, MPI_COMM_WORLD, &status);
                        nodo_ganador = static_cast<int>(mensaje_exito[0]);
                        hilo_ganador = static_cast<int>(mensaje_exito[1]);
                        indice_encontrado = mensaje_exito[2];
                        encontrado_global = true;
                    }
                }

                for (int i = 1; i < size; i++) {
                    MPI_Send(&encontrado_global, 1, MPI_C_BOOL, i, 200, MPI_COMM_WORLD);
                }

                double tiempo_fin = MPI_Wtime();
                tiempo_paralelo = tiempo_fin - tiempo_inicio;
                if (tiempo_paralelo == 0.0) tiempo_paralelo = 1e-6;

                convertirIndiceATexto(indice_encontrado, clave_encontrada, longitud_clave);

                cout << "\n================ RESULTADO PARALELO DISTRIBUIDO ================\n";
                cout << " Clave Descifrada con Exito: " << clave_encontrada << "\n";
                cout << " Nodo Ganador: " << nodo_ganador << " | Hilo OpenMP: " << hilo_ganador << "\n";
                cout << " Indice Global: " << indice_encontrado << "\n";
                cout << " Tiempo Paralelo Total: " << tiempo_paralelo << " segundos.\n";
                cout << "=================================================================\n";

                log_file << "[EXITO] [Equipo: " << pc_name << "] [Nodo Maestro] [Clave Encontrada: \"" << clave_encontrada << "\"] [Tiempo Total: " << tiempo_paralelo << "s]" << endl;
                log_file << "[Finalizado] Coordinacion maestra completada." << endl;

            }
            else {
                bool encontrado_local = false;
                unsigned long long idx_resultado = 0;
                int hilo_resultado = -1;
                double t_esclavo_inicio = MPI_Wtime();
                bool modo_detallado = (longitud_clave <= 3);

#pragma omp parallel shared(encontrado_local, idx_resultado, hilo_resultado)
                {
#pragma omp for schedule(dynamic, 1000)
                    for (long long idx = static_cast<long long>(inicio_local); idx <= static_cast<long long>(fin_local); idx++) {
                        if (encontrado_local) continue;

                        char candidato[11];
                        convertirIndiceATexto(idx, candidato, longitud_clave);

                        if (modo_detallado && omp_get_thread_num() == 0 && (idx % 100 == 0)) {
#pragma omp critical
                            {
                                log_file << "[Equipo: " << pc_name << "] [Nodo MPI: " << rank
                                    << "] [Hilo OpenMP: 0] [Candidato: " << candidato << "] [Estado: Evaluando]" << endl;
                            }
                        }

                        if (strcmp(candidato, clave_objetivo) == 0) {
#pragma omp critical
                            {
                                if (!encontrado_local) {
                                    encontrado_local = true;
                                    idx_resultado = idx;
                                    hilo_resultado = omp_get_thread_num();
                                    double t_parcial = MPI_Wtime() - t_esclavo_inicio;

                                    unsigned long long datos_exito[3] = {
                                        static_cast<unsigned long long>(rank),
                                        static_cast<unsigned long long>(hilo_resultado),
                                        idx_resultado
                                    };
                                    MPI_Send(datos_exito, 3, MPI_UNSIGNED_LONG_LONG, 0, 100, MPI_COMM_WORLD);

                                    log_file << "[EXITO] [Equipo: " << pc_name << "] [Nodo MPI: " << rank
                                        << "] [Hilo OpenMP: " << hilo_resultado << "] [Clave Encontrada: \"" << candidato
                                        << "\"] [Tiempo Parcial: " << t_parcial << "s]" << endl;
                                }
                            }
                        }
                    }
                }

                bool parada = false;
                MPI_Status status;
                MPI_Recv(&parada, 1, MPI_C_BOOL, 0, 200, MPI_COMM_WORLD, &status);

                log_file << "[Finalizado] Proceso esclavo completado y finalizado limpiamente." << endl;
            }

            log_file.close();

        }
        else if (opcion == 4) {
            if (rank == 0) {
                cout << "\n================ TABLA COMPARATIVA DE RENDIMIENTO ================\n";
                cout << " Tiempo Secuencial (T1):  " << tiempo_secuencial << " segundos\n";
                cout << " Tiempo Paralelo (Tp):    " << tiempo_paralelo << " segundos\n";

                if (tiempo_paralelo > 0 && tiempo_secuencial > 0) {
                    double speedup = tiempo_secuencial / tiempo_paralelo;
                    double eficiencia = speedup / size;
                    cout << " Ganancia de Velocidad (Speedup Sp): " << speedup << "\n";
                    cout << " Eficiencia (Ep):                    " << eficiencia << " (" << (eficiencia * 100.0) << "%)\n";
                }
                else {
                    cout << " [Aviso] Ejecute previamente las opciones 2 y 3.\n";
                }
                cout << "===================================================================\n";
            }
        }

    } while (true);

    if (clave_objetivo != nullptr) {
        delete[] clave_objetivo;
        clave_objetivo = nullptr;
    }

    if (rank == 0) {
        imprimirIntegrantes();
    }

    MPI_Finalize();
    return 0;
}