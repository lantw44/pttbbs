/* $Id$ */
#include "bbs.h"
#include "chc.h"

#define assert_not_reached() assert(!"Should never be here!!!")

extern const double elo_exp_tab[1000];

enum Turn {
    BLK = 0,
    RED 
};

enum Kind {
    KIND_K=1,
    KIND_A,
    KIND_E,
    KIND_R,
    KIND_H,
    KIND_C,
    KIND_P,
};
#define CENTER(a, b)	(((a) + (b)) >> 1)
#define CHC_TIMEOUT	300

#define PHOTO_LINE      15
#define PHOTO_COLUMN    (256 + 25)

typedef struct drc_t {
    ChessStepType   type;  /* necessary one */
    rc_t            from, to;
}               drc_t;

typedef struct {
    rc_t select;
    char selected;
} chc_tag_data_t;

/* chess framework action functions */
static void chc_init_user(const userinfo_t *uinfo, ChessUser *user);
static void chc_init_board(board_t board);
static void chc_drawline(const ChessInfo* info, int line);
static void chc_movecur(int r, int c);
static void chc_prepare_play(ChessInfo* info);
static int  chc_select(ChessInfo* info, rc_t scrloc, ChessGameResult* result);
static void chc_prepare_step(ChessInfo* info, const void* step);
static ChessGameResult chc_movechess(board_t board, const drc_t* move);
static void chc_drawstep(ChessInfo* info, const drc_t* move);
static void chc_gameend(ChessInfo* info, ChessGameResult result);
static void chc_genlog(ChessInfo* info, FILE* fp, ChessGameResult result);


static const char * const turn_color[2]={BLACK_COLOR, RED_COLOR};

/* some constant variable definition */

static const char * const turn_str[2] = {"黑的", "紅的"};

static const char * const num_str[2][10] = {
    {"", "１", "２", "３", "４", "５", "６", "７", "８", "９"},
    {"", "一", "二", "三", "四", "五", "六", "七", "八", "九"},
};

static const char * const chess_str[2][8] = {
    /* 0     1     2     3     4     5     6     7 */
    {"  ", "將", "士", "象", "車", "馬", "包", "卒"},
    {"  ", "帥", "仕", "相", "車", "傌", "炮", "兵"}
};

static const char * const chess_brd[BRD_ROW * 2 - 1] = {
    /* 0   1   2   3   4   5   6   7   8 */
    "┌─┬─┬─┬─┬─┬─┬─┬─┐",	/* 0 */
    "│  │  │  │＼│／│  │  │  │",
    "├─┼─┼─┼─┼─┼─┼─┼─┤",	/* 1 */
    "│  │  │  │／│＼│  │  │  │",
    "├─┼─┼─┼─┼─┼─┼─┼─┤",	/* 2 */
    "│  │  │  │  │  │  │  │  │",
    "├─┼─┼─┼─┼─┼─┼─┼─┤",	/* 3 */
    "│  │  │  │  │  │  │  │  │",
    "├─┴─┴─┴─┴─┴─┴─┴─┤",	/* 4 */
    "│  楚    河          漢    界  │",
    "├─┬─┬─┬─┬─┬─┬─┬─┤",	/* 5 */
    "│  │  │  │  │  │  │  │  │",
    "├─┼─┼─┼─┼─┼─┼─┼─┤",	/* 6 */
    "│  │  │  │  │  │  │  │  │",
    "├─┼─┼─┼─┼─┼─┼─┼─┤",	/* 7 */
    "│  │  │  │＼│／│  │  │  │",
    "├─┼─┼─┼─┼─┼─┼─┼─┤",	/* 8 */
    "│  │  │  │／│＼│  │  │  │",
    "└─┴─┴─┴─┴─┴─┴─┴─┘"	/* 9 */
};

static char * const hint_str[] = {
    "  q      認輸離開",
    "  p      要求和棋",
    "方向鍵   移動遊標",
    "Enter    選擇/移動"
};

static const ChessActions chc_actions = {
    &chc_init_user,
    (void (*) (void*)) &chc_init_board,
    &chc_drawline,
    &chc_movecur,
    &chc_prepare_play,
    &chc_select,
    &chc_prepare_step,
    (ChessGameResult (*) (void*, const void*)) &chc_movechess,
    (void (*)(ChessInfo*, const void*)) &chc_drawstep,
    &chc_gameend,
    &chc_genlog
};

static const ChessConstants chc_constants = {
    sizeof(drc_t),
    CHC_TIMEOUT,
    BRD_ROW,
    BRD_COL,
    "楚河漢界",
    "photo_cchess",
#ifdef GLOBAL_CCHESS_LOG
    GLOBAL_CCHESS_LOG,
#else
    NULL,
#endif
    { BLACK_COLOR, RED_COLOR },
    {"黑的", "紅的"}
};

/*
 * Start of the drawing function.
 */
static void
chc_movecur(int r, int c)
{
    move(r * 2 + 3, c * 4 + 4);
}

static char *
getstep(board_t board, const rc_t *from, const rc_t *to, char buf[])
{
    int             turn, fc, tc;
    char           *dir;
    int		    twin = 0, twin_r = 0;
    int		    len = 0;

    turn = CHE_O(board[from->r][from->c]);
    if(CHE_P(board[from->r][from->c] != KIND_P)) { // TODO 目前不管兵卒前後
	int i;
	for(i=0;i<10;i++)
	    if(board[i][from->c]==board[from->r][from->c]) {
		if(i!=from->r) {
		    twin=1;
		    twin_r=i;
		}
	    }
    }
    fc = (turn == BLK ? from->c + 1 : 9 - from->c);
    tc = (turn == BLK ? to->c + 1 : 9 - to->c);
    if (from->r == to->r)
	dir = "平";
    else {
	if (from->c == to->c)
	    tc = from->r - to->r;
	if (tc < 0)
	    tc = -tc;

	if ((turn == BLK && to->r > from->r) ||
	    (turn == RED && to->r < from->r))
	    dir = "進";
	else
	    dir = "退";
    }


    len=sprintf(buf, "%s", turn_color[turn]);
    /* 傌二|前傌 */
    if(twin) {
	len+=sprintf(buf+len, "%s%s",
		((from->r>twin_r)==(turn==(BLK)))?"前":"後",
		chess_str[turn][CHE_P(board[from->r][from->c])]);
    } else {
	len+=sprintf(buf+len, "%s%s",
		chess_str[turn][CHE_P(board[from->r][from->c])],
		num_str[turn][fc]);
    }
    /* 進三 */
    len+=sprintf(buf+len, "%s%s" ANSI_RESET, dir, num_str[turn][tc]);
    /* ：象 */
    if(board[to->r][to->c]) {
	len+=sprintf(buf+len,"：%s%s" ANSI_RESET,
		turn_color[turn^1],
		chess_str[turn^1][CHE_P(board[to->r][to->c])]);
    }
    return buf;
}

inline static const char*
chc_timestr(int second)
{
    static char str[10];
    snprintf(str, sizeof(str), "%d:%02d", second / 60, second % 60);
    return str;
}

static void
chc_drawline(const ChessInfo* info, int line)
{
    int             i, j;
    board_p         board = (board_p) info->board;
    chc_tag_data_t *tag = info->tag;

    move(line, 0);
    clrtoeol();
    if (line == 0) {
	prints(ANSI_COLOR(1;46) "   象棋對戰   " ANSI_COLOR(45)
		"%30s VS %-20s%10s" ANSI_RESET,
	       info->user1.userid, info->user2.userid,
	       info->mode == CHESS_MODE_WATCH ? "[觀棋模式]" : "");
    } else if (line >= 3 && line <= 21) {
	outs("   ");
	for (i = 0; i < 9; i++) {
	    j = board[RTL(info,line)][CTL(info,i)];
	    if ((line & 1) == 1 && j) {
		if (tag->selected &&
		    tag->select.r == RTL(info,line) && tag->select.c == CTL(info,i)) {
		    prints("%s%s" ANSI_RESET,
			   CHE_O(j) == BLK ? BLACK_REVERSE : RED_REVERSE,
			   chess_str[CHE_O(j)][CHE_P(j)]);
		}
		else {
		    prints("%s%s" ANSI_RESET,
			   turn_color[CHE_O(j)],
			   chess_str[CHE_O(j)][CHE_P(j)]);
		}
	    } else
		prints("%c%c", chess_brd[line - 3][i * 4],
		       chess_brd[line - 3][i * 4 + 1]);
	    if (i != 8)
		prints("%c%c", chess_brd[line - 3][i * 4 + 2],
		       chess_brd[line - 3][i * 4 + 3]);
	}
    } else if (line == 2 || line == 22) {
	outs("   ");
	if (line == 2)
	    for (i = 1; i <= 9; i++)
		prints("%s  ", num_str[REDDOWN(info)?0:1][i]);
	else
	    for (i = 9; i >= 1; i--)
		prints("%s  ", num_str[REDDOWN(info)?1:0][i]);
    }

    ChessDrawExtraInfo(info, line);
}
/*
 * End of the drawing function.
 */


/*
 * Start of the log function.
 */
void
chc_log_step(FILE* fp, board_t board, const drc_t *step)
{
    char buf[80];
    buf[0] = buf[1] = ' ';
    getstep(board, &step->from, &step->to, buf + 2);
    fputs(buf, fp);
    fputc('\n', fp);
}

static int
#if defined(__linux__)
chc_filter(const struct dirent *dir)
#else
chc_filter(struct dirent *dir)
#endif
{
    if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0 )
	return 0;
    return strstr(dir->d_name, ".poem") != NULL;
}

static int
chc_log_poem(FILE* outfp)
{
    struct dirent **namelist;
    int n;

    // TODO use readdir(), don't use lots of memory
    n = scandir(BBSHOME"/etc/chess", &namelist, chc_filter, alphasort);
    if (n < 0)
	perror("scandir");
    else {
	char buf[80];
	FILE *fp;
	sprintf(buf, BBSHOME"/etc/chess/%s", namelist[random() % n]->d_name);
	if ((fp = fopen(buf, "r")) == NULL)
	    return -1;

	while(fgets(buf, sizeof(buf), fp) != NULL)
	    fputs(buf, outfp);
	while(n--)
	    free(namelist[n]);
	free(namelist);
	fclose(fp);
    }
    return 0;
}

static void
chc_genlog(ChessInfo* info, FILE* fp, ChessGameResult result)
{
    const int nStep = info->history.used;
    board_t   board;
    int i;

    if (info->myturn == RED)
	fprintf(fp, "%s V.S. %s\n", info->user1.userid, info->user2.userid);
    else
	fprintf(fp, "%s V.S. %s\n", info->user2.userid, info->user1.userid);

    chc_init_board(board);
    for (i = 0; i < nStep; ++i) {
	const drc_t *move = (const drc_t*)  ChessHistoryRetrieve(info, i);
	if (move->type == CHESS_STEP_NORMAL) {
	    chc_log_step(fp, board, move);
	    chc_movechess(board, move);
	}
    }

    if (result == CHESS_RESULT_TIE)
	fprintf(fp, "=> 和局\n");
    else if (result == CHESS_RESULT_WIN || result == CHESS_RESULT_LOST)
	fprintf(fp, "=> %s 勝\n",
		(info->myturn == RED) == (result== CHESS_RESULT_WIN) ?
		"紅" : "黑");
    
    fputs("\n--\n\n", fp);

    /* TODO: generate machine readable log.
     * e.g. http://www.nchess.com/ccff.html */

    chc_log_poem(fp);
}
/*
 * End of the log function.
 */


/*
 * Start of the rule function.
 */
static void
chc_init_board(board_t board)
{
    memset(board, 0, sizeof(board_t));
    board[0][4] = CHE(KIND_K, BLK);	/* 將 */
    board[0][3] = board[0][5] = CHE(KIND_A, BLK);	/* 士 */
    board[0][2] = board[0][6] = CHE(KIND_E, BLK);	/* 象 */
    board[0][0] = board[0][8] = CHE(KIND_R, BLK);	/* 車 */
    board[0][1] = board[0][7] = CHE(KIND_H, BLK);	/* 馬 */
    board[2][1] = board[2][7] = CHE(KIND_C, BLK);	/* 包 */
    board[3][0] = board[3][2] = board[3][4] =
	board[3][6] = board[3][8] = CHE(KIND_P, BLK);	/* 卒 */

    board[9][4] = CHE(KIND_K, RED);	/* 帥 */
    board[9][3] = board[9][5] = CHE(KIND_A, RED);	/* 仕 */
    board[9][2] = board[9][6] = CHE(KIND_E, RED);	/* 相 */
    board[9][0] = board[9][8] = CHE(KIND_R, RED);	/* 車 */
    board[9][1] = board[9][7] = CHE(KIND_H, RED);	/* 傌 */
    board[7][1] = board[7][7] = CHE(KIND_C, RED);	/* 炮 */
    board[6][0] = board[6][2] = board[6][4] =
	board[6][6] = board[6][8] = CHE(KIND_P, RED);	/* 兵 */
}

static void
chc_prepare_step(ChessInfo* info, const void* step)
{
    const drc_t* move = (const drc_t*) step;
    getstep((board_p) info->board, 
	    &move->from, &move->to, info->last_movestr);
}

static ChessGameResult
chc_movechess(board_t board, const drc_t* move)
{
    int end = (CHE_P(board[move->to.r][move->to.c]) == KIND_K);

    board[move->to.r][move->to.c] = board[move->from.r][move->from.c];
    board[move->from.r][move->from.c] = 0;

    return end ? CHESS_RESULT_WIN : CHESS_RESULT_CONTINUE;
}

static void
chc_drawstep(ChessInfo* info, const drc_t* move)
{
    ChessDrawLine(info, LTR(info, move->from.r));
    ChessDrawLine(info, LTR(info, move->to.r));
}

/* 求兩座標行或列(rowcol)的距離 */
static int
dist(rc_t from, rc_t to, int rowcol)
{
    int             d;

    d = rowcol ? from.c - to.c : from.r - to.r;
    return d > 0 ? d : -d;
}

/* 兩座標(行或列rowcol)中間有幾顆棋子 */
static int
between(board_t board, rc_t from, rc_t to, int rowcol)
{
    int             i, rtv = 0;

    if (rowcol) {
	if (from.c > to.c)
	    i = from.c, from.c = to.c, to.c = i;
	for (i = from.c + 1; i < to.c; i++)
	    if (board[to.r][i])
		rtv++;
    } else {
	if (from.r > to.r)
	    i = from.r, from.r = to.r, to.r = i;
	for (i = from.r + 1; i < to.r; i++)
	    if (board[i][to.c])
		rtv++;
    }
    return rtv;
}

static int
chc_canmove(board_t board, rc_t from, rc_t to)
{
    int             i;
    int             rd, cd, turn;

    if(0 ||
	!(0<=from.r && from.r<BRD_ROW) ||
	!(0<=from.c && from.c<BRD_COL) ||
	!(0<=to.r && to.r<BRD_ROW) ||
	!(0<=to.c && to.c<BRD_COL))
	return 0;

    rd = dist(from, to, 0);
    cd = dist(from, to, 1);
    turn = CHE_O(board[from.r][from.c]);

    /* general check */
    if (board[to.r][to.c] && CHE_O(board[to.r][to.c]) == turn)
	return 0;

    /* individual check */
    switch (CHE_P(board[from.r][from.c])) {
    case KIND_K:		/* 將 帥 */
	if (!(rd == 1 && cd == 0) &&
	    !(rd == 0 && cd == 1))
	    return 0;
	if ((turn == BLK && to.r > 2) ||
	    (turn == RED && to.r < 7) ||
	    to.c < 3 || to.c > 5)
	    return 0;
	break;
    case KIND_A:		/* 士 仕 */
	if (!(rd == 1 && cd == 1))
	    return 0;
	if ((turn == BLK && to.r > 2) ||
	    (turn == RED && to.r < 7) ||
	    to.c < 3 || to.c > 5)
	    return 0;
	break;
    case KIND_E:		/* 象 相 */
	if (!(rd == 2 && cd == 2))
	    return 0;
	if ((turn == BLK && to.r > 4) ||
	    (turn == RED && to.r < 5))
	    return 0;
	/* 拐象腿 */
	if (board[CENTER(from.r, to.r)][CENTER(from.c, to.c)])
	    return 0;
	break;
    case KIND_R:		/* 車 */
	if (!(rd > 0 && cd == 0) &&
	    !(rd == 0 && cd > 0))
	    return 0;
	if (between(board, from, to, rd == 0))
	    return 0;
	break;
    case KIND_H:		/* 馬 傌 */
	if (!(rd == 2 && cd == 1) &&
	    !(rd == 1 && cd == 2))
	    return 0;
	/* 拐馬腳 */
	if (rd == 2) {
	    if (board[CENTER(from.r, to.r)][from.c])
		return 0;
	} else {
	    if (board[from.r][CENTER(from.c, to.c)])
		return 0;
	}
	break;
    case KIND_C:		/* 包 炮 */
	if (!(rd > 0 && cd == 0) &&
	    !(rd == 0 && cd > 0))
	    return 0;
	i = between(board, from, to, rd == 0);
	if ((i > 1) ||
	    (i == 1 && !board[to.r][to.c]) ||
	    (i == 0 && board[to.r][to.c]))
	    return 0;
	break;
    case KIND_P:		/* 卒 兵 */
	if (!(rd == 1 && cd == 0) &&
	    !(rd == 0 && cd == 1))
	    return 0;
	if (((turn == BLK && to.r < 5) ||
	     (turn == RED && to.r > 4)) &&
	    cd != 0)
	    return 0;
	if ((turn == BLK && to.r < from.r) ||
	    (turn == RED && to.r > from.r))
	    return 0;
	break;
    }
    return 1;
}

/* 找 turn's king 的座標 */
static int
findking(board_t board, int turn, rc_t * buf)
{
    int             i, r, c;

    r = (turn == BLK ? 0 : 7);
    for (i = 0; i < 3; r++, i++)
	for (c = 3; c < 6; c++)
	    if (CHE_P(board[r][c]) == KIND_K &&
		CHE_O(board[r][c]) == turn) {
		buf->r = r, buf->c = c;
		return 1;
	    }
    /* one's king may be eaten */
    return 0;
}

static int
chc_iskfk(board_t board)
{
    rc_t            from, to;

    if (!findking(board, BLK, &to)) return 0;
    if (!findking(board, RED, &from)) return 0;
    if (from.c == to.c && between(board, from, to, 0) == 0)
	return 1;
    return 0;
}

static int
chc_ischeck(board_t board, int turn)
{
    rc_t            from, to;

    if (!findking(board, turn, &to)) return 0;
    for (from.r = 0; from.r < BRD_ROW; from.r++)
	for (from.c = 0; from.c < BRD_COL; from.c++)
	    if (board[from.r][from.c] &&
		CHE_O(board[from.r][from.c]) != turn)
		if (chc_canmove(board, from, to))
		    return 1;
    return 0;
}
/*
 * End of the rule function.
 */

static void
chcusr_put(userec_t* userec, const ChessUser* user)
{
    userec->chc_win = user->win;
    userec->chc_lose = user->lose;
    userec->chc_tie = user->tie;
    userec->chess_elo_rating = user->rating;
}

static void
chc_init_user(const userinfo_t *uinfo, ChessUser *user)
{
    strlcpy(user->userid, uinfo->userid, sizeof(user->userid));
    user->win    = uinfo->chc_win;
    user->lose   = uinfo->chc_lose;
    user->tie    = uinfo->chc_tie;
    user->rating = uinfo->chess_elo_rating;
    if(user->rating == 0)
	user->rating = 1500; /* ELO initial value */
    user->orig_rating = user->rating;
}


static void
chc_prepare_play(ChessInfo* info)
{
    if (chc_ischeck((board_p) info->board, info->turn)) {
	strlcpy(info->warnmsg, ANSI_COLOR(1;31) "將軍!" ANSI_RESET,
		sizeof(info->warnmsg));
	bell();
    } else
	info->warnmsg[0] = 0;
}

static int
chc_select(ChessInfo* info, rc_t scrloc, ChessGameResult* result)
{
    chc_tag_data_t* tag = (chc_tag_data_t*) info->tag;
    board_p board       = (board_p)         info->board;
    rc_t loc;

    assert(tag);

    /* transform from screen to internal coordinate */
    if(REDDOWN(info)) {
      loc = scrloc;
    } else {
      loc.r = BRD_ROW-scrloc.r-1;
      loc.c = BRD_COL-scrloc.c-1;
    }

    if (!tag->selected) {
	/* trying to pick something */
	if (board[loc.r][loc.c] &&
		CHE_O(board[loc.r][loc.c]) == info->turn) {
	    /* they can pick up this */
	    tag->selected = 1;
	    tag->select = loc;
	    ChessDrawLine(info, LTR(info, loc.r));
	}
	return 0;
    } else if (tag->select.r == loc.r && tag->select.c == loc.c) {
	/* cancel selection */
	tag->selected = 0;
	ChessDrawLine(info, LTR(info, loc.r));
	return 0;
    } else if (chc_canmove(board, tag->select, loc)) {
	/* moving the chess */
	drc_t   moving = { CHESS_STEP_NORMAL, tag->select, loc };
	board_t tmpbrd;
	int valid_step = 1;

	if (CHE_P(board[loc.r][loc.c]) == KIND_K)
	    /* 移到對方將帥 */
	    *result = CHESS_RESULT_WIN;
	else {
	    memcpy(tmpbrd, board, sizeof(board_t));
	    chc_movechess(tmpbrd, &moving);
	    valid_step = !chc_iskfk(tmpbrd);
	}

	if (valid_step) {
	    getstep(board, &moving.from, &moving.to, info->last_movestr);

	    chc_movechess(board, &moving);
	    ChessDrawLine(info, LTR(info, moving.from.r));
	    ChessDrawLine(info, LTR(info, moving.to.r));

	    ChessHistoryAppend(info, &moving);
	    ChessStepSend(info, &moving);

	    tag->selected = 0;
	    return 1;
	} else {
	    /* 王見王 */
	    strlcpy(info->warnmsg,
		    ANSI_COLOR(1;33) "不可以王見王" ANSI_RESET,
		    sizeof(info->warnmsg));
	    bell();
	    ChessDrawLine(info, CHESS_DRAWING_WARN_ROW);
	    return 0;
	}
    } else
	/* nothing happened */
	return 0;
}

int round_to_int(double x)
{
    /* assume that double cast to int will drop fraction parts */
    if(x>=0)
	return (int)(x+0.5);
    return (int)(x-0.5);
}

/*
 * ELO rating system
 * see http://www.wordiq.com/definition/ELO_rating_system
 */
static void
count_chess_elo_rating(ChessUser* user1, const ChessUser* user2, double myres)
{
    double k;
    double exp_res;
    int diff;
    int newrating;

    if(user1->rating < 1800)
	k = 30;
    else if(user1->rating < 2000)
	k = 25;
    else if(user1->rating < 2200)
	k = 20;
    else if(user1->rating < 2400)
	k = 15;
    else
	k = 10;

    //exp_res = 1.0/(1.0 + pow(10.0, (user2->rating-user1->rating)/400.0));
    //user1->rating += (int)floor(k*(myres-exp_res)+0.5);
    diff=(int)user2->rating-(int)user1->rating;
    if(diff<=-1000 || diff>=1000)
       exp_res=diff>0?0.0:1.0;
    else if(diff>=0)
       exp_res=elo_exp_tab[diff];
    else
       exp_res=1.0-elo_exp_tab[-diff];
    newrating = (int)user1->rating + round_to_int(k*(myres-exp_res));
    if(newrating > 3000) newrating = 3000;
    if(newrating < 1) newrating = 1;
    user1->rating = newrating;
}


/* 象棋功能進入點:
 * chc_main: 對奕
 * chc_personal: 打譜
 * chc_watch: 觀棋
 * talk.c: 對奕
 */
void
chc(int s, ChessGameMode mode)
{
    ChessInfo*     info = NewChessInfo(&chc_actions, &chc_constants, s, mode);
    board_t        board;
    chc_tag_data_t tag;

    chc_init_board(board);
    tag.selected = 0;

    info->board = board;
    info->tag   = &tag;

    if (info->mode == CHESS_MODE_VERSUS) {
	/* Assume that info->user1 is me. */
	info->user1.lose++;
	count_chess_elo_rating(&info->user1, &info->user2, 0.0);
	passwd_query(usernum, &cuser);
	chcusr_put(&cuser, &info->user1);
	passwd_update(usernum, &cuser);
    }

    if (mode == CHESS_MODE_WATCH)
	setutmpmode(CHESSWATCHING);
    else
	setutmpmode(CHC);
    currutmp->sig = SIG_CHC;

    ChessPlay(info);

    DeleteChessInfo(info);
}

static void
chc_gameend(ChessInfo* info, ChessGameResult result)
{
    ChessUser* const user1 = &info->user1;
    ChessUser* const user2 = &info->user2;

    if (info->mode == CHESS_MODE_VERSUS) {
	if (info->myturn == RED) {
	    /* 由紅方作 log. 記的是下棋前的原始分數 */
	    /* NOTE, 若紅方斷線則無 log */
	    time_t t = time(NULL);
	    char buf[100];
	    sprintf(buf, "%s %s(%d,W%d/D%d/L%d) %s %s(%d,W%d/D%d/L%d)\n",
		    ctime(&t),
		    user1->userid, user1->rating, user1->win,
		    user1->tie, user1->lose - 1,
		    (result == CHESS_RESULT_TIE ? "和" :
		     result == CHESS_RESULT_WIN ? "勝" : "負"),
		    user2->userid, user2->rating, user2->win,
		    user2->tie, user2->lose - 1);
	    buf[24] = ' '; // replace '\n'
	    log_file(BBSHOME "/log/chc.log", LOG_CREAT, buf);
	}

	user1->rating = user1->orig_rating;
	user1->lose--;
	if (result == CHESS_RESULT_WIN) {
	    count_chess_elo_rating(user1, user2, 1.0);
	    user1->win++;
	    currutmp->chc_win++;
	} else if (result == CHESS_RESULT_LOST) {
	    count_chess_elo_rating(user1, user2, 0.0);
	    user1->lose++;
	    currutmp->chc_lose++;
	} else {
	    count_chess_elo_rating(user1, user2, 0.5);
	    user1->tie++;
	    currutmp->chc_tie++;
	}
	currutmp->chess_elo_rating = user1->rating;
	chcusr_put(&cuser, user1);
	passwd_update(usernum, &cuser);
    }
}

int
chc_main(void)
{
    return ChessStartGame('c', SIG_CHC, "楚河漢界之爭");
}

int
chc_personal(void)
{
    chc(0, CHESS_MODE_PERSONAL);
    return 0;
}

int
chc_watch(void)
{
    return ChessWatchGame(&chc, CHC, "楚河漢界之爭");
}
