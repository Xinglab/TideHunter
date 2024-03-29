#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "tidehunter.h"
#include "edlib_align.h"
#include "abpoa_cons.h"
#include "ksw2_align.h"
#include "utils.h"

void write_tandem_cons_seq(tandem_seq_t *tseq, char *cons_seq, uint8_t *cons_qual, int cons_len, int start, int end, double copy_num, double ave_match, mini_tandem_para *mtp, int8_t full_length, int *par_pos, int pos_n) {
    if (cons_len < mtp->min_len || cons_len > mtp->max_p) return;
    if (mtp->only_longest && tseq->cons_n == 1) {
        if (end-start > tseq->cons_end[0]-tseq->cons_start[0]) {
            tseq->cons_n = 0; tseq->cons_seq->seq.l = 0;
        } else return;
    }
    if (tseq->cons_seq->seq.l + cons_len >= tseq->cons_seq->seq.m) {
        tseq->cons_seq->seq.m = tseq->cons_seq->seq.l + cons_len + 1;
        tseq->cons_seq->seq.s = (char*)_err_realloc(tseq->cons_seq->seq.s, tseq->cons_seq->seq.m * sizeof(char));
    }
    strncpy(tseq->cons_seq->seq.s + tseq->cons_seq->seq.l, cons_seq, cons_len); tseq->cons_seq->seq.l += cons_len; 
    if (cons_qual) {
        if (tseq->cons_seq->qual.l + cons_len >= tseq->cons_seq->qual.m) {
            tseq->cons_seq->qual.m = tseq->cons_seq->qual.l + cons_len + 1;
            tseq->cons_seq->qual.s = (char*)_err_realloc(tseq->cons_seq->qual.s, tseq->cons_seq->qual.m * sizeof(char));
        }
        int i;
        for (i = 0; i < cons_len; ++i)
            tseq->cons_seq->qual.s[tseq->cons_seq->qual.l+i] = cons_qual[i];
        tseq->cons_seq->qual.l += cons_len; 
    }

    int i;
    if (tseq->cons_n == tseq->cons_m) {
        tseq->cons_m <<= 1;
        tseq->cons_start = (int*)_err_realloc(tseq->cons_start, tseq->cons_m * sizeof(int));
        tseq->cons_end = (int*)_err_realloc(tseq->cons_end, tseq->cons_m * sizeof(int));
        tseq->copy_num = (double*)_err_realloc(tseq->copy_num, tseq->cons_m * sizeof(double));
        tseq->ave_match = (double*)_err_realloc(tseq->ave_match, tseq->cons_m * sizeof(double));
        tseq->full_length = (int8_t*)_err_realloc(tseq->full_length, tseq->cons_m * sizeof(int8_t));
        tseq->cons_len = (int*)_err_realloc(tseq->cons_len, tseq->cons_m * sizeof(int));
        tseq->cons_score = (int*)_err_realloc(tseq->cons_score, tseq->cons_m * sizeof(int));
        tseq->sub_pos = (int**)_err_realloc(tseq->sub_pos, tseq->cons_m * sizeof(int*));
        tseq->pos_n = (int*)_err_realloc(tseq->pos_n, tseq->cons_m * sizeof(int));
        tseq->pos_m = (int*)_err_realloc(tseq->pos_m, tseq->cons_m * sizeof(int));
        for (i = tseq->cons_n; i < tseq->cons_m; ++i) {
            tseq->pos_m[i] = 0; tseq->pos_n[i] = 0;
            tseq->sub_pos[i] = 0;
        }
    }
    tseq->cons_start[tseq->cons_n] = start; tseq->cons_end[tseq->cons_n] = end; tseq->copy_num[tseq->cons_n] = copy_num;
    tseq->full_length[tseq->cons_n] = full_length;
    tseq->cons_len[tseq->cons_n] = cons_len; // tseq->cons_score[tseq->cons_n] = 0; // TODO cons_score
    tseq->ave_match[tseq->cons_n] = ave_match;
    tseq->pos_n[tseq->cons_n] = pos_n;
    if (pos_n > tseq->pos_m[tseq->cons_n]) {
        tseq->pos_m[tseq->cons_n] = pos_n;
        tseq->sub_pos[tseq->cons_n] = (int*)_err_realloc(tseq->sub_pos[tseq->cons_n], pos_n * sizeof(int));
    }
    for (i = 0; i < pos_n; ++i) tseq->sub_pos[tseq->cons_n][i] = par_pos[i];
    ++tseq->cons_n;
}

void write_tandem_unit(tandem_seq_t *tseq, int *par_pos, int pos_n) {
    int i;
    if (tseq->cons_n == tseq->cons_m) {
        tseq->cons_m <<= 1;
        tseq->sub_pos = (int**)_err_realloc(tseq->sub_pos, tseq->cons_m * sizeof(int*));
        tseq->pos_n = (int*)_err_realloc(tseq->pos_n, tseq->cons_m * sizeof(int));
        tseq->pos_m = (int*)_err_realloc(tseq->pos_m, tseq->cons_m * sizeof(int));
        for (i = tseq->cons_n; i < tseq->cons_m; ++i) {
            tseq->pos_m[i] = 0; tseq->pos_n[i] = 0;
            tseq->sub_pos[i] = 0;
        }
    }
    tseq->pos_n[tseq->cons_n] = pos_n;
    if (pos_n > tseq->pos_m[tseq->cons_n]) {
        tseq->pos_m[tseq->cons_n] = pos_n;
        tseq->sub_pos[tseq->cons_n] = (int*)_err_realloc(tseq->sub_pos[tseq->cons_n], pos_n * sizeof(int));
    }
    for (i = 0; i < pos_n; ++i) tseq->sub_pos[tseq->cons_n][i] = par_pos[i];
    ++tseq->cons_n;
}

typedef struct {
    int ed, start, end;
} ed_res_t;

int collect_ed_res(mini_tandem_para *mtp, char *q, int qlen, char *seq, int seq_len, ed_res_t *res) {
    int n = 0, ed, start, end;
    ed = edlib_align_HW(q, qlen, seq, seq_len, &start, &end, qlen * (1-mtp->ada_match_rat));
    if (ed != -1) {
        res[0].ed = ed; res[0].start = start; res[0].end = end; n++;
        // 2nd
        if (res[0].start >= qlen) {
            ed = edlib_align_HW(q, qlen, seq, res[0].start, &start, &end, qlen * (1-mtp->ada_match_rat));
            if (ed != -1) {
                res[n].ed = ed; res[n].start = start; res[n].end = end; n++;
            }
        }
        // 3rd
        if (res[0].end <= seq_len-qlen) {
            ed = edlib_align_HW(q, qlen, seq+res[0].end, seq_len-res[0].end, &start, &end, qlen * (1-mtp->ada_match_rat));
            if (ed != -1) {
                res[n].ed = ed; res[n].start = res[0].end+start; res[n].end = res[0].end+end; n++;
            }
        }
    }
    return n;
}

int get_full_len_seq(mini_tandem_para *mtp, int left_n, ed_res_t *left_res, int right_n, ed_res_t *right_res, int *tar_start, int *tar_end) {
    int tot_ed = INT32_MAX;
    int i, j;
    for (i = 0; i < left_n; ++i) {
        for (j = 0; j < right_n; ++j) {
            if (right_res[j].start - left_res[i].end - 1 >= mtp->min_len) {
                if (tot_ed > left_res[i].ed + right_res[j].ed) {
                    tot_ed = left_res[i].ed + right_res[j].ed;
                    *tar_start = left_res[i].end + 1; *tar_end = right_res[j].start - 1;
                }
            }
        }
    }
    return tot_ed;
}

void single_copy_full_len_seq(int seq_len, char *seq, tandem_seq_t *tseq, mini_tandem_para *mtp) {
    int cons_len=0; 

    // |---plus-5'adapter---|---target-sequence---|---minus-3'adapter---|
    int full_length = 0, tar_start, tar_end, tot_ed;
    ed_res_t *_5_ed_res = (ed_res_t*)_err_malloc(3 * sizeof(ed_res_t));
    ed_res_t *_3_ed_res = (ed_res_t*)_err_malloc(3 * sizeof(ed_res_t));
    int _5_n, _3_n;
    int par_pos[2];
    tar_start = tar_end = -1; tot_ed = INT32_MAX;
    // XXX all _5/_3 in the seq
    // choose the pair with smallest ed and with the correct direction
    // collect at most 3 pos of _5/_3
    _5_n = collect_ed_res(mtp, mtp->five_seq, mtp->five_len, seq, seq_len, _5_ed_res);
    _3_n = collect_ed_res(mtp, mtp->three_rc_seq, mtp->three_len, seq, seq_len, _3_ed_res);
    tot_ed = get_full_len_seq(mtp, _5_n, _5_ed_res, _3_n, _3_ed_res, &tar_start, &tar_end); 
    if (tot_ed != INT32_MAX) {
        par_pos[0] = tar_start; par_pos[1] = tar_end; cons_len = tar_end-tar_start+1;
        full_length = 1;
    }
    if (tot_ed > 0) { // try rev-comp
        _5_n = collect_ed_res(mtp, mtp->five_rc_seq, mtp->five_len, seq, seq_len, _5_ed_res);
        _3_n = collect_ed_res(mtp, mtp->three_seq, mtp->three_len, seq, seq_len, _3_ed_res);
        if (get_full_len_seq(mtp, _3_n, _3_ed_res, _5_n, _5_ed_res, &tar_start, &tar_end) < tot_ed) {
            par_pos[0] = tar_start; par_pos[1] = tar_end; cons_len = tar_end-tar_start+1;
            full_length = 2;
        }
    }
    
    // write single copy cons
    if (full_length > 0) {
        if (mtp->only_unit) write_tandem_unit(tseq, par_pos, 2);
        else {
            int i; uint8_t *cons_qual = NULL;
            if (mtp->out_fmt == FASTQ_FMT || mtp->out_fmt == TAB_QUAL_FMT) {
                cons_qual = (uint8_t*)_err_malloc(cons_len * sizeof(uint8_t));
                for (i = 0; i < cons_len; ++i) cons_qual[i] = 33;
            }
            write_tandem_cons_seq(tseq, seq+par_pos[0], cons_qual, cons_len, par_pos[0], par_pos[1], 1.0, 100.0, mtp, full_length, par_pos, 2);
            if (cons_qual != NULL) free(cons_qual); 
        }
    }
    free(_5_ed_res); free(_3_ed_res);
}

void seqs_msa(int seq_len, uint8_t *bseq, int par_n, int *par_pos, tandem_seq_t *tseq, mini_tandem_para *mtp, abpoa_t *ab, abpoa_para_t *abpt) {
        int cons_len=0;
        char *cons_seq = (char*)_err_malloc(seq_len * sizeof(char));
        uint8_t *cons_bseq = (uint8_t*)_err_malloc(seq_len * sizeof(uint8_t)), *cons_qual = NULL;
        if (mtp->out_fmt == FASTQ_FMT || mtp->out_fmt == TAB_QUAL_FMT) cons_qual = (uint8_t*)_err_malloc(seq_len * sizeof(uint8_t));
#ifdef __DEBUG__
        {
            int i, j, seq_i = 0, start, end;
            for (i = 0; i < par_n-1; ++i) {
                if (par_pos[i] >= 0 && par_pos[i+1] >= 0) {
                    start = par_pos[i], end = par_pos[i+1];
                    printf(">seqs_%d:%d-%d\n", end-start, start, end);
                    for (j = start+1; j <= end; ++j) printf("%c", "ACGTN"[bseq[j]]);
                    printf("\n");
                }
            }
        }
#endif
        int i = 0, j, k, s;
        while (i < par_n-mtp->min_copy) {
            if (par_pos[i] < 0) {
                i++;
                continue;
            }
            for (j = i+1; j < par_n; ++j) {
                if (par_pos[j] < 0) break;
            }
            if (j - i > mtp->min_copy) { // do multiple sequence alignment and consensus calling for par_pos[i:j]
                if (mtp->only_unit) {
                    write_tandem_unit(tseq, par_pos+i, j-i);
                } else {
                    int n_seqs;
                    cons_len = abpoa_gen_cons(ab, abpt, bseq, seq_len, par_pos+i, j-i, cons_bseq, cons_qual, mtp, &n_seqs);
                    if (cons_len == 0) continue;
                    // invoke ksw2_global to calculate iden_n / unit_len 
                    double ave_match = 0; int start, end, len, iden_n;
                    for (k = i; k < j-1; ++k) {
                        start = par_pos[k]; end = par_pos[k+1]; len = end - start;
                        iden_n = ksw2_global(bseq+start+1, len, cons_bseq, cons_len);
                        // printf("%d %d, %d\n", iden_n, len, cons_len);
                        ave_match += (iden_n * 100 / (len+0.0));
                    }
                    for (s = 0; s < cons_len; ++s) cons_seq[s] = "ACGTN"[cons_bseq[s]]; cons_seq[cons_len] = '\0';

                    int max_q, max_t, cons_start, cons_end; double copy_num = n_seqs;
                    ksw2_left_ext(cons_bseq, cons_len, bseq, par_pos[i]+1, &max_q, &max_t); cons_start = par_pos[i] - max_t;
                    // printf("max_q: %d, max_t: %d\n", max_q, max_t);
                    copy_num += (max_q + 1.0) / cons_len;
                    ksw2_right_ext(cons_bseq, cons_len, bseq+par_pos[j-1]+1, seq_len-par_pos[j-1]-1, &max_q, &max_t); cons_end = par_pos[j-1] + max_t + 1;
                    // printf("max_q: %d, max_t: %d\n", max_q, max_t);
                    copy_num += (max_q + 1.0) / cons_len;
                    // find full-length sequence based on 5' and 3' adapter sequences
                    int full_length = 0;
                    if (mtp->five_seq != NULL && mtp->three_seq != NULL && cons_len > mtp->five_len + mtp->three_len) {
                        char *cons2 = (char*)_err_malloc(((cons_len << 1) + 1) * sizeof(char)); uint8_t *qual2 = NULL;
                        strcpy(cons2, cons_seq); strcpy(cons2+cons_len, cons_seq); cons2[cons_len<<1] = '\0'; // concatenated 2 copies
                        if (cons_qual) { 
                            qual2 = (uint8_t*)_err_malloc((cons_len << 1) * sizeof(uint8_t));
                            for (k = 0; k < cons_len; ++k) qual2[k] = cons_qual[k];
                            for (k = 0; k < cons_len; ++k) qual2[cons_len+k] = cons_qual[k];
                        }
                        int tar_start, tar_end, tot_ed, _5_ed, _3_ed, _5_start, _5_end, _3_start, _3_end;
                        tar_start = tar_end = -1;
                        // |---plus-5'adapter---|---target-sequence---|---minus-3'adapter---|
                        _5_start = _5_end = _3_start = _3_end = -1; tot_ed = INT32_MAX;
                        _5_ed = edlib_align_HW(mtp->five_seq, mtp->five_len, cons2, cons_len<<1, &_5_start, &_5_end, mtp->five_len * (1-mtp->ada_match_rat));
                        if (_5_ed == -1) goto REV;
                        _3_ed = edlib_align_HW(mtp->three_rc_seq, mtp->three_len, cons2, cons_len<<1, &_3_start, &_3_end, mtp->three_len * (1-mtp->ada_match_rat));
                        if (_3_ed == -1) goto REV; 
                        if (_3_start <= _5_end) {
                            if (_3_end + cons_len < cons_len << 1 && _3_start + cons_len > _5_end) {
                                tar_start = _5_end + 1;
                                tar_end = _3_start + cons_len - 1;
                                full_length = 1;
                                tot_ed = _5_ed + _3_ed;
                            }
                        } else {
                            tar_start = _5_end + 1;
                            tar_end = _3_start - 1;
                            tot_ed = _5_ed + _3_ed;
                            full_length = 1;
                        }
#ifdef __DEBUG__
                        printf("FOR: %d, %d => %d, %d\n", tar_start, tar_end, _5_ed, _3_ed);
#endif
                        if (tot_ed == 0) goto WRITE_CONS;
                        // |---plus-3'adapter---|---target-sequence---|---minus-5'adapter---|
REV:
                        _5_ed = edlib_align_HW(mtp->five_rc_seq, mtp->five_len, cons2, cons_len<<1, &_5_start, &_5_end, mtp->five_len * (1-mtp->ada_match_rat));
                        if (_5_ed == -1) goto WRITE_CONS;
                        _3_ed = edlib_align_HW(mtp->three_seq, mtp->three_len, cons2, cons_len<<1, &_3_start, &_3_end, mtp->three_len * (1-mtp->ada_match_rat));
                        if (_3_ed == -1) goto WRITE_CONS; 
                        if (_5_ed + _3_ed < tot_ed) {
                            if (_5_start <= _3_end) {
                                if (_5_end + cons_len < cons_len << 1 && _5_start + cons_len > _3_end) {
                                    tar_start = _3_end + 1;
                                    tar_end = _5_start + cons_len - 1;
                                    full_length = 2;
                                }
                            } else {
                                tar_start = _3_end + 1;
                                tar_end = _5_start - 1;
                                full_length = 2;
                            }
#ifdef __DEBUG__
                            printf("REV: %d, %d => %d\n", tar_start, tar_end, _5_ed + _3_ed);
#endif
                        }
WRITE_CONS:
                        if (tar_start > 0 && tar_end > tar_start) {
                            memcpy(cons_seq, cons2 + tar_start , tar_end - tar_start + 1);
                            cons_seq[tar_end - tar_start + 1] = '\0';
                            if (cons_qual) {
                                for (k = tar_start; k <= tar_end; ++k) cons_qual[k-tar_start] = qual2[k];
                            }
                            cons_len = tar_end - tar_start + 1;
                        }
                        free(cons2); if (cons_qual) free(qual2);
                    }
                    // write cons_seq into tseq
                    if (!(mtp->only_full_length) || full_length > 0)
                        write_tandem_cons_seq(tseq, cons_seq, cons_qual, cons_len, cons_start, cons_end, copy_num, ave_match/(j-i-1), mtp, full_length, par_pos+i, j-i);
                }
            }
            i = j + 1;
        }
        free(cons_seq); free(cons_bseq); free(par_pos);
        if (cons_qual) free(cons_qual);
}
