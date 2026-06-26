#include "presentation.h"
#include "auth.h"
#include "avl.h"
#include "user.h"
#include "file.h"
#include "heap.h"
#include "huffman.h"
#include "storage.h"
#include "chat.h"
#include "graph.h"
#include "report.h"

static void clear_input_buffer(void)
{
    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF)
        ;
}

static int get_int(const char *prompt, int *value)
{
    int r;

    while (1)
    {
        printf("%s", prompt);
        r = scanf("%d", value);
        if (r == EOF)
        {
            printf("\nA sair...\n");
            return EOF;
        }
        if (r == 1)
        {
            clear_input_buffer();
            return 1;
        }
        printf("Entrada inválida. Tente novamente.\n");
        clear_input_buffer();
    }
}

static int get_line(const char *prompt, char *buf, int size)
{
    while (1)
    {
        printf("%s", prompt);
        if (fgets(buf, size, stdin) == NULL)
        {
            printf("\nA sair...\n");
            return EOF;
        }
        buf[strcspn(buf, "\r\n")] = '\0';
        if (buf[0] != '\0')
            return 1;
        printf("Entrada inválida. Tente novamente.\n");
    }
}

static void separator(void)
{
    printf("  ------------------------------------------------\n");
}

static void header(const char *title)
{
    printf("\n  ================================================\n");
    printf("  %s\n", title);
    printf("  ================================================\n\n");
}

static void pause_enter(void)
{
    int c;
    printf("\n  Prima ENTER para continuar...");
    fflush(stdout);
    while ((c = getchar()) != '\n' && c != EOF)
        ;
}

static void clear_screen(void)
{
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

static User *prompt_user(Auth *auth, const char *prompt, int *eof)
{
    int id;
    int r = get_int(prompt, &id);
    if (r == EOF)
    {
        *eof = 1;
        return NULL;
    }
    User *u = find_user(auth->users, id);
    if (!u)
        printf("Utilizador com ID %d não encontrado.\n", id);
    return u;
}

int menu_create_user(Auth *auth)
{
    int  id;
    char name[100];

    if (get_int("ID: ", &id) != 1)
        return 0;
    if (get_line("Nome: ", name, sizeof(name)) != 1)
        return 0;

    if (find_user(auth->users, id))
    {
        printf("Utilizador com ID %d já existe.\n", id);
        return 1;
    }
    User *user = create_user(id, name);
    if (!user)
    {
        printf("Erro ao criar utilizador.\n");
        return 0;
    }
    auth->users = insert_user(auth->users, user);
    printf("Utilizador '%s' criado.\n", name);
    return 1;
}

int menu_remove_user(Auth *auth)
{
    int id;

    if (get_int("ID a remover: ", &id) != 1)
        return 0;
    if (!find_user(auth->users, id))
    {
        printf("Utilizador não encontrado.\n");
        return 1;
    }
    if (auth->current_user && auth->current_user->id == id)
    {
        printf("Não é possível remover o utilizador com sessão activa.\n");
        return 1;
    }
    auth->users = delete_user(auth->users, id);
    printf("Utilizador removido.\n");
    return 1;
}

int menu_find_user(Auth *auth)
{
    int id;

    if (get_int("ID a procurar: ", &id) != 1)
        return 0;
    User *user = find_user(auth->users, id);
    if (!user)
        printf("Utilizador não encontrado.\n");
    else
        printf("ID: %d | Nome: %s | Bloqueado: %s\n",
               user->id, user->name, user->blocked ? "Sim" : "Não");
    return 1;
}

int menu_block_user(Auth *auth)
{
    int id;

    if (get_int("ID a bloquear: ", &id) != 1)
        return 0;
    User *user = find_user(auth->users, id);
    if (!user)
        printf("Utilizador não encontrado.\n");
    else
    {
        block_user(user);
        printf("Utilizador bloqueado.\n");
    }
    return 1;
}

int menu_unblock_user(Auth *auth)
{
    int id;

    if (get_int("ID a desbloquear: ", &id) != 1)
        return 0;
    User *user = find_user(auth->users, id);
    if (!user)
        printf("Utilizador não encontrado.\n");
    else
    {
        unblock_user(user);
        printf("Utilizador desbloqueado.\n");
    }
    return 1;
}

int menu_login(Auth *auth)
{
    int id;

    if (get_int("ID: ", &id) != 1)
        return 0;
    login(auth, id);
    return 1;
}

int menu_add_file(Auth *auth)
{
    if (!auth->current_user)
    {
        printf("Faça login primeiro.\n");
        return 1;
    }

    int  id;
    char name[256];

    if (get_int("ID do ficheiro: ", &id) != 1)
        return 0;
    if (get_line("Nome do ficheiro: ", name, sizeof(name)) != 1)
        return 0;

    auth->current_user->files =
        adicionar_ficheiro(auth->current_user->files, id, name, NULL);
    printf("Ficheiro '%s' adicionado.\n", name);
    return 1;
}

int menu_remove_file(Auth *auth)
{
    if (!auth->current_user)
    {
        printf("Faça login primeiro.\n");
        return 1;
    }

    int id;

    if (get_int("ID do ficheiro a remover: ", &id) != 1)
        return 0;

    if (!procurar_por_id(auth->current_user->files, id))
    {
        printf("Ficheiro não encontrado.\n");
        return 1;
    }
    auth->current_user->files =
        remover_ficheiro(auth->current_user->files, id);
    printf("Ficheiro removido.\n");
    return 1;
}

int menu_list_files(Auth *auth)
{
    if (!auth->current_user)
    {
        printf("Faça login primeiro.\n");
        return 1;
    }
    if (!auth->current_user->files)
    {
        printf("Nenhum ficheiro registado.\n");
        return 1;
    }
    listar_ficheiros(auth->current_user->files);
    return 1;
}

int menu_huffman_compress(Auth *auth)
{
    (void)auth;

    char texto[1000];
    char saida[5000];
    char codigo[MAX_CODE];

    if (get_line("Texto a comprimir: ", texto, sizeof(texto)) != 1)
        return 0;

    int freq[MAX_CHAR] = {0};
    for (int i = 0; texto[i]; i++)
        freq[(unsigned char)texto[i]]++;

    for (int i = 0; i < MAX_CHAR; i++)
        codigos[i][0] = '\0';

    No *raiz = construirArvore(freq);
    if (!raiz)
    {
        printf("Erro ao construir a árvore de Huffman.\n");
        return 1;
    }

    printf("\n  Tabela de códigos:\n");
    gerarCodigos(raiz, codigo, 0);
    comprimir(texto, saida);
    int orig = tamanhoOriginal(texto);
    int comp = tamanhoComprimido(texto);
    float taxa = (orig > 0) ? taxaCompressao(orig, comp) : 0.0f;
    printf("\n  Texto original  : %s\n", texto);
    printf("  Bits comprimidos: %s\n", saida);
    printf("  Tamanho original : %d bits\n", orig);
    printf("  Tamanho comprimido: %d bits\n", comp);
    printf("  Taxa de compressão: %.2f%%\n", taxa);
    printf("  Verificação (descompressão):\n  ");
    descomprimir(raiz, saida);
    Registro r;
    strncpy(r.textoOriginal, texto, sizeof(r.textoOriginal) - 1);
    r.textoOriginal[sizeof(r.textoOriginal) - 1] = '\0';
    strncpy(r.comprimido, saida, sizeof(r.comprimido) - 1);
    r.comprimido[sizeof(r.comprimido) - 1] = '\0';
    r.tamanhoOriginal   = orig;
    r.tamanhoComprimido = comp;
    salvarRegistro(r);
    printf("  Registo guardado em dados.txt\n");
    (void)raiz;

    return 1;
}

static int chat_enviar(Auth *auth)
{
    header("CHAT > Enviar Mensagem");

    int eof = 0;
    User *s = prompt_user(auth, "ID do remetente: ", &eof);
    if (eof) return 0;
    if (!s)  return 1;

    User *r = prompt_user(auth, "ID do destinatário: ", &eof);
    if (eof) return 0;
    if (!r)  return 1;

    if (s->blocked || r->blocked)
    {
        printf("Um dos utilizadores está bloqueado.\n");
        return 1;
    }

    char content[MAX_MESSAGE_LEN];
    if (get_line("Mensagem: ", content, sizeof(content)) != 1)
        return 0;

    int id = chat_send(auth->message, auth->chat, s, r, content);
    printf(id > 0 ? "\n  Mensagem #%d enviada!\n" : "\n  Erro ao enviar.\n", id);
    return 1;
}

static int chat_receber(Auth *auth)
{
    header("CHAT > Receber Próxima Mensagem");
    if (queue_is_empty(auth->message))
    {
        printf("  Fila vazia. Nenhuma mensagem para receber.\n");
        return 1;
    }
    Message *m = chat_receive(auth->message);
    if (m)
    {
        char buf[32];
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&m->timestamp));
        separator();
        printf("  ID        : #%d\n", m->id);
        printf("  De        : %s\n", m->sender->name);
        printf("  Para      : %s\n", m->receiver->name);
        printf("  Mensagem  : %s\n", m->content);
        printf("  Timestamp : %s\n", buf);
        separator();
        free(m);
    }
    return 1;
}

static int chat_historico(Auth *auth)
{
    header("CHAT > Histórico Completo");
    if (auth->chat->count == 0)
    {
        printf("  Sem mensagens.\n");
        return 1;
    }
    history_print(auth->chat);
    return 1;
}

static int chat_historico_user(Auth *auth)
{
    header("CHAT > Histórico por Utilizador");
    int eof = 0;
    User *u = prompt_user(auth, "ID do utilizador: ", &eof);
    if (eof) return 0;
    if (!u)  return 1;
    history_print_user(auth->chat, u);
    return 1;
}

static void chat_status(Auth *auth)
{
    header("CHAT > Estado da Fila");
    printf("  Mensagens na fila : %d\n", auth->message->size);
    printf("  Total no histórico: %d\n", auth->chat->count);
}

int menu_chat(Auth *auth)
{
    int op;

    do
    {
        clear_screen();
        printf("\n  ================================================\n");
        printf("  MÓDULO DE CHAT\n");
        printf("  ================================================\n");
        printf("  [1] Enviar mensagem\n");
        printf("  [2] Receber próxima mensagem da fila\n");
        printf("  [3] Ver histórico completo\n");
        printf("  [4] Ver histórico por utilizador\n");
        printf("  [5] Estado da fila\n");
        printf("  [0] Voltar ao menu principal\n");
        separator();

        if (get_int("  Opção: ", &op) == EOF)
            return 0;

        switch (op)
        {
            case 1: if (!chat_enviar(auth))         return 0; pause_enter(); break;
            case 2: if (!chat_receber(auth))        return 0; pause_enter(); break;
            case 3: if (!chat_historico(auth))      return 0; pause_enter(); break;
            case 4: if (!chat_historico_user(auth)) return 0; pause_enter(); break;
            case 5: chat_status(auth);                         pause_enter(); break;
            case 0: break;
            default:
                printf("  Opção inválida.\n");
                pause_enter();
                break;
        }
    } while (op != 0);

    return 1;
}

static int graph_registar(Auth *auth)
{
    header("PARTILHAS > Registar Partilha");

    int eof = 0;
    User *s = prompt_user(auth, "ID do remetente: ", &eof);
    if (eof) return 0;
    if (!s)  return 1;

    User *r = prompt_user(auth, "ID do destinatário: ", &eof);
    if (eof) return 0;
    if (!r)  return 1;

    if (!s->files)
    {
        printf("  O utilizador '%s' não tem ficheiros.\n", s->name);
        return 1;
    }

    /* Listar ficheiros do remetente */
    printf("  Ficheiros de %s:\n", s->name);
    listar_ficheiros(s->files);

    int fid;
    if (get_int("  ID do ficheiro a partilhar: ", &fid) != 1)
        return 0;

    File *file = procurar_por_id(s->files, fid);
    if (!file)
    {
        printf("  Ficheiro não encontrado.\n");
        return 1;
    }

    if (graph_add_share(auth->graph, s, r, file) == 0)
        printf("\n  Partilha registada!\n");
    else
        printf("\n  Erro ao registar partilha.\n");
    return 1;
}

static int graph_add_user_menu(Auth *auth)
{
    header("PARTILHAS > Adicionar Utilizador ao Grafo");
    int eof = 0;
    User *u = prompt_user(auth, "ID do utilizador: ", &eof);
    if (eof) return 0;
    if (!u)  return 1;

    int idx = graph_add_user(auth->graph, u);
    if (idx >= 0)
        printf("\n  Utilizador '%s' registado no grafo (índice %d).\n", u->name, idx);
    else
        printf("\n  Erro ao registar utilizador.\n");
    return 1;
}

static void graph_listar(Auth *auth)
{
    header("PARTILHAS > Grafo de Partilhas");
    if (auth->graph->user_count == 0)
    {
        printf("  Sem dados.\n");
        return;
    }
    graph_print(auth->graph);
}

static void graph_users(Auth *auth)
{
    header("PARTILHAS > Utilizadores Registados no Grafo");
    if (auth->graph->user_count == 0)
    {
        printf("  Nenhum utilizador.\n");
        return;
    }
    for (int i = 0; i < auth->graph->user_count; i++)
        printf("  [%d] %s\n", i, auth->graph->users[i]->name);
    printf("\n  Total: %d\n", auth->graph->user_count);
}

static int graph_dfs_menu(Auth *auth)
{
    header("PARTILHAS > Pesquisa DFS");
    if (auth->graph->user_count == 0)
    {
        printf("  Sem utilizadores no grafo.\n");
        return 1;
    }
    int eof = 0;
    User *u = prompt_user(auth, "ID do utilizador de partida: ", &eof);
    if (eof) return 0;
    if (!u)  return 1;
    graph_dfs(auth->graph, u);
    return 1;
}

static int graph_bfs_menu(Auth *auth)
{
    header("PARTILHAS > Pesquisa BFS");
    if (auth->graph->user_count == 0)
    {
        printf("  Sem utilizadores no grafo.\n");
        return 1;
    }
    int eof = 0;
    User *u = prompt_user(auth, "ID do utilizador de partida: ", &eof);
    if (eof) return 0;
    if (!u)  return 1;
    graph_bfs(auth->graph, u);
    return 1;
}

int menu_graph(Auth *auth)
{
    int op;

    do
    {
        clear_screen();
        printf("\n  ================================================\n");
        printf("  MÓDULO DE PARTILHAS\n");
        printf("  ================================================\n");
        printf("  [1] Registar partilha\n");
        printf("  [2] Adicionar utilizador ao grafo\n");
        printf("  [3] Ver grafo de partilhas\n");
        printf("  [4] Ver utilizadores no grafo\n");
        printf("  [5] Pesquisa DFS\n");
        printf("  [6] Pesquisa BFS\n");
        printf("  [0] Voltar ao menu principal\n");
        separator();

        if (get_int("  Opção: ", &op) == EOF)
            return 0;

        switch (op)
        {
            case 1: if (!graph_registar(auth))      return 0; pause_enter(); break;
            case 2: if (!graph_add_user_menu(auth)) return 0; pause_enter(); break;
            case 3: graph_listar(auth);                        pause_enter(); break;
            case 4: graph_users(auth);                         pause_enter(); break;
            case 5: if (!graph_dfs_menu(auth))      return 0; pause_enter(); break;
            case 6: if (!graph_bfs_menu(auth))      return 0; pause_enter(); break;
            case 0: break;
            default:
                printf("  Opção inválida.\n");
                pause_enter();
                break;
        }
    } while (op != 0);

    return 1;
}

int menu_reports(Auth *auth)
{
    int op;

    do
    {
        clear_screen();
        printf("\n  ================================================\n");
        printf("  MÓDULO DE RELATÓRIOS\n");
        printf("  ================================================\n");
        printf("  [1] Partilhas por utilizador\n");
        printf("  [2] Utilizador que mais partilhou\n");
        printf("  [3] Utilizador que mais recebeu\n");
        printf("  [4] Ficheiros mais partilhados\n");
        printf("  [5] Total de membros\n");
        printf("  [6] Todos os relatórios\n");
        printf("  [0] Voltar ao menu principal\n");
        separator();

        if (get_int("  Opção: ", &op) == EOF)
            return 0;

        if (op >= 1 && op <= 6 && auth->graph->user_count == 0)
        {
            printf("\n  Sem dados. Registe utilizadores e partilhas primeiro.\n");
            pause_enter();
            continue;
        }

        switch (op)
        {
            case 1:
                header("Partilhas por Utilizador");
                report_shares_per_user(auth->graph);
                pause_enter();
                break;
            case 2:
                header("Utilizador que Mais Partilhou");
                report_top_sender(auth->graph);
                pause_enter();
                break;
            case 3:
                header("Utilizador que Mais Recebeu");
                report_top_receiver(auth->graph);
                pause_enter();
                break;
            case 4:
                header("Ficheiros Mais Partilhados");
                report_most_shared_files(auth->graph);
                pause_enter();
                break;
            case 5:
                header("Total de Membros");
                report_total_members(auth->graph);
                pause_enter();
                break;
            case 6:
                report_all(auth->graph);
                pause_enter();
                break;
            case 0:
                break;
            default:
                printf("  Opção inválida.\n");
                pause_enter();
                break;
        }
    } while (op != 0);

    return 1;
}

void print_menu(Auth *auth)
{
    printf("\n=== Sistema de Partilha de Ficheiros ===\n");
    if (auth->current_user)
        printf("Sessão : %s (ID %d)\n",
               auth->current_user->name, auth->current_user->id);
    else
        printf("Sessão : nenhuma\n");
    printf("Chat   : %d mensagem(ns) na fila | "
           "Grafo: %d utilizador(es) | "
           "%d partilha(s)\n",
           auth->message->size,
           auth->graph->user_count,
           auth->graph->total_shares);

    printf("\n-- Membros --\n");
    printf(" 1. Criar membro          2. Remover membro\n");
    printf(" 3. Procurar membro       4. Listar membros\n");
    printf(" 5. Bloquear membro       6. Desbloquear membro\n");

    printf("\n-- Sessão --\n");
    printf(" 7. Login                 8. Logout\n");

    printf("\n-- Ficheiros --\n");
    printf(" 9. Adicionar ficheiro   10. Remover ficheiro\n");
    printf("11. Listar ficheiros\n");

    printf("\n-- Compressão Huffman --\n");
    printf("12. Comprimir texto\n");

    printf("\n-- Chat --\n");
    printf("13. Módulo de Chat\n");

    printf("\n-- Partilhas --\n");
    printf("14. Módulo de Partilhas\n");

    printf("\n-- Relatórios --\n");
    printf("15. Relatórios\n");

    printf("\n 0. Sair\n");
    printf("Opção: ");
}