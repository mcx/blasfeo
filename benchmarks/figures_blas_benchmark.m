% print blas benchmark



% DGEMM

printf('dgemm\n');

f1 = figure();

dgemm_nn
save -mat dgemm_nn.mat A B

plot(B(:,1), B(:,2));

dgemm_nt
save -mat dgemm_nt.mat A B

hold all
plot(B(:,1), B(:,2));
hold off

dgemm_tn
save -mat dgemm_tn.mat A B

hold all
plot(B(:,1), B(:,2));
hold off

dgemm_tt
save -mat dgemm_tt.mat A B

hold all
plot(B(:,1), B(:,2));
hold off

Gflops_max = A(1)*A(2);

axis([0 300 0 Gflops_max]);
legend('dgemm\_nn', 'dgemm\_nt', 'dgemm\_tn', 'dgemm\_tt', 'Location', 'SouthEast');
xlabel('matrix size n')
ylabel('Gflops')
grid on
ytick = get(gca, 'ytick');
ytick = [ytick, Gflops_max];
set(gca, 'ytick', ytick);

file_name = ['dgemm'];
file_name_eps = [file_name, '.eps'];
file_name_pdf = [file_name, '.pdf'];
print(f1, file_name_eps, '-depsc') 
system(['epstopdf ', file_name_eps, ' -out ', file_name_pdf]);
system(['rm ', file_name_eps]);



% DSYRK

printf('dsyrk\n');

f1 = figure();
dsyrk_ln
dsyrk_ln
save -mat dsyrk_ln.mat A B

plot(B(:,1), B(:,2));

Gflops_max = A(1)*A(2);

axis([0 300 0 Gflops_max]);
legend('dsyrk\_ln', 'Location', 'SouthEast');
xlabel('matrix size n')
ylabel('Gflops')
grid on
ytick = get(gca, 'ytick');
ytick = [ytick, Gflops_max];
set(gca, 'ytick', ytick);

file_name = ['dsyrk'];
file_name_eps = [file_name, '.eps'];
file_name_pdf = [file_name, '.pdf'];
print(f1, file_name_eps, '-depsc') 
system(['epstopdf ', file_name_eps, ' -out ', file_name_pdf]);
system(['rm ', file_name_eps]);



% DTRMM

printf('dtrmm\n');

f1 = figure();

dtrmm_rlnn
save -mat dtrmm_rlnn.mat A B

plot(B(:,1), B(:,2));

dtrmm_rutn
save -mat dtrmm_rutn.mat A B

hold all
plot(B(:,1), B(:,2));
hold off

Gflops_max = A(1)*A(2);

axis([0 300 0 Gflops_max]);
legend('dtrmm\_rlnn', 'dtrmm\_rutn', 'Location', 'SouthEast');
xlabel('matrix size n')
ylabel('Gflops')
grid on
ytick = get(gca, 'ytick');
ytick = [ytick, Gflops_max];
set(gca, 'ytick', ytick);

file_name = ['dtrmm'];
file_name_eps = [file_name, '.eps'];
file_name_pdf = [file_name, '.pdf'];
print(f1, file_name_eps, '-depsc') 
system(['epstopdf ', file_name_eps, ' -out ', file_name_pdf]);
system(['rm ', file_name_eps]);



% DTRSM

printf('dtrsm\n');

f1 = figure();

dtrsm_llnu
save -mat dtrsm_llnu.mat A B

plot(B(:,1), B(:,2));

dtrsm_lunn
save -mat dtrsm_lunn.mat A B

hold all
plot(B(:,1), B(:,2));
hold off

dtrsm_rltn
save -mat dtrsm_rltn.mat A B

hold all
plot(B(:,1), B(:,2));
hold off

dtrsm_rltu
save -mat dtrsm_rltu.mat A B

hold all
plot(B(:,1), B(:,2));
hold off

dtrsm_rutn
save -mat dtrsm_rutn.mat A B

hold all
plot(B(:,1), B(:,2));
hold off

Gflops_max = A(1)*A(2);

axis([0 300 0 Gflops_max]);
legend('dtrsm\_llnu', 'dtrsm\_lunn', 'dtrsm\_rltn', 'dtrsm\_rltu', 'dtrsm\_rutn', 'Location', 'SouthEast');
xlabel('matrix size n')
ylabel('Gflops')
grid on
ytick = get(gca, 'ytick');
ytick = [ytick, Gflops_max];
set(gca, 'ytick', ytick);

file_name = ['dtrsm'];
file_name_eps = [file_name, '.eps'];
file_name_pdf = [file_name, '.pdf'];
print(f1, file_name_eps, '-depsc') 
system(['epstopdf ', file_name_eps, ' -out ', file_name_pdf]);
system(['rm ', file_name_eps]);



% DGEQRF

printf('dgeqrf\n');

f1 = figure();

dgelqf
save -mat dgelqf.mat A B

plot(B(:,1), B(:,2));

dgeqrf
save -mat dgeqrf.mat A B

hold all
plot(B(:,1), B(:,2));
hold off

Gflops_max = A(1)*A(2);

axis([0 300 0 Gflops_max]);
legend('dgelqf', 'dgeqrf', 'Location', 'SouthEast');
xlabel('matrix size n')
ylabel('Gflops')
grid on
ytick = get(gca, 'ytick');
ytick = [ytick, Gflops_max];
set(gca, 'ytick', ytick);

file_name = ['dgeqrf'];
file_name_eps = [file_name, '.eps'];
file_name_pdf = [file_name, '.pdf'];
print(f1, file_name_eps, '-depsc') 
system(['epstopdf ', file_name_eps, ' -out ', file_name_pdf]);
system(['rm ', file_name_eps]);



% DGETRF

printf('dgetrf\n');

f1 = figure();

dgetrf_nopivot
save -mat dgetrf_nopivot.mat A B

plot(B(:,1), B(:,2));

dgetrf_rowpivot
save -mat dgetrf_rowpivot.mat A B

hold all
plot(B(:,1), B(:,2));
hold off

Gflops_max = A(1)*A(2);

axis([0 300 0 Gflops_max]);
legend('dgetrf\_nopivot', 'dgetrf\_rowpivot', 'Location', 'SouthEast');
xlabel('matrix size n')
ylabel('Gflops')
grid on
ytick = get(gca, 'ytick');
ytick = [ytick, Gflops_max];
set(gca, 'ytick', ytick);

file_name = ['dgetrf'];
file_name_eps = [file_name, '.eps'];
file_name_pdf = [file_name, '.pdf'];
print(f1, file_name_eps, '-depsc') 
system(['epstopdf ', file_name_eps, ' -out ', file_name_pdf]);
system(['rm ', file_name_eps]);



% DPOTRF

printf('dpotrf\n');

f1 = figure();

dpotrf_l
save -mat dpotrf_l.mat A B

plot(B(:,1), B(:,2));

dpotrf_u
save -mat dpotrf_u.mat A B

hold all
plot(B(:,1), B(:,2));
hold off

Gflops_max = A(1)*A(2);

axis([0 300 0 Gflops_max]);
legend('dpotrf\_l', 'dpotrf\_u', 'Location', 'SouthEast');
xlabel('matrix size n')
ylabel('Gflops')
grid on
ytick = get(gca, 'ytick');
ytick = [ytick, Gflops_max];
set(gca, 'ytick', ytick);

file_name = ['dpotrf'];
file_name_eps = [file_name, '.eps'];
file_name_pdf = [file_name, '.pdf'];
print(f1, file_name_eps, '-depsc') 
system(['epstopdf ', file_name_eps, ' -out ', file_name_pdf]);
system(['rm ', file_name_eps]);



% DGEMV

printf('dgemv\n');

f1 = figure();

dgemv_n
save -mat dgemv_n.mat A B

plot(B(:,1), B(:,2));

dgemv_t
save -mat dgemv_t.mat A B

hold all
plot(B(:,1), B(:,2));
hold off

dgemv_nt
save -mat dgemv_nt.mat A B

hold all
plot(B(:,1), B(:,2));
hold off

Gflops_max = A(1)*A(2);

axis([0 300 0 Gflops_max]);
legend('dgemv\_n', 'dgemv\_t', 'dgemv\_nt', 'Location', 'SouthEast');
xlabel('matrix size n')
ylabel('Gflops')
grid on
ytick = get(gca, 'ytick');
ytick = [ytick, Gflops_max];
set(gca, 'ytick', ytick);

file_name = ['dgemv'];
file_name_eps = [file_name, '.eps'];
file_name_pdf = [file_name, '.pdf'];
print(f1, file_name_eps, '-depsc') 
system(['epstopdf ', file_name_eps, ' -out ', file_name_pdf]);
system(['rm ', file_name_eps]);



% DTRMV

printf('dtrmv\n');

f1 = figure();

dtrmv_lnn
save -mat dtrmv_lnn.mat A B

plot(B(:,1), B(:,2));

dtrmv_ltn
save -mat dtrmv_ltn.mat A B

hold all
plot(B(:,1), B(:,2));
hold off

Gflops_max = A(1)*A(2);

axis([0 300 0 Gflops_max]);
legend('dtrmv\_lnn', 'dtrmv\_ltn', 'Location', 'SouthEast');
xlabel('matrix size n')
ylabel('Gflops')
grid on
ytick = get(gca, 'ytick');
ytick = [ytick, Gflops_max];
set(gca, 'ytick', ytick);

file_name = ['dtrmv'];
file_name_eps = [file_name, '.eps'];
file_name_pdf = [file_name, '.pdf'];
print(f1, file_name_eps, '-depsc') 
system(['epstopdf ', file_name_eps, ' -out ', file_name_pdf]);
system(['rm ', file_name_eps]);



% DTRSV

printf('dtrsv\n');

f1 = figure();

dtrsv_lnn
save -mat dtrsv_lnn.mat A B

plot(B(:,1), B(:,2));

dtrsv_ltn
save -mat dtrsv_ltn.mat A B

hold all
plot(B(:,1), B(:,2));
hold off

Gflops_max = A(1)*A(2);

axis([0 300 0 Gflops_max]);
legend('dtrsv\_lnn', 'dtrsv\_ltn', 'Location', 'SouthEast');
xlabel('matrix size n')
ylabel('Gflops')
grid on
ytick = get(gca, 'ytick');
ytick = [ytick, Gflops_max];
set(gca, 'ytick', ytick);

file_name = ['dtrsv'];
file_name_eps = [file_name, '.eps'];
file_name_pdf = [file_name, '.pdf'];
print(f1, file_name_eps, '-depsc') 
system(['epstopdf ', file_name_eps, ' -out ', file_name_pdf]);
system(['rm ', file_name_eps]);



% DSYMV

printf('dsymv\n');

f1 = figure();

dsymv_l
save -mat dsymv_l.mat A B

plot(B(:,1), B(:,2));

Gflops_max = A(1)*A(2);

axis([0 300 0 Gflops_max]);
legend('dsymv\_l', 'Location', 'SouthEast');
xlabel('matrix size n')
ylabel('Gflops')
grid on
ytick = get(gca, 'ytick');
ytick = [ytick, Gflops_max];
set(gca, 'ytick', ytick);

file_name = ['dsymv'];
file_name_eps = [file_name, '.eps'];
file_name_pdf = [file_name, '.pdf'];
print(f1, file_name_eps, '-depsc') 
system(['epstopdf ', file_name_eps, ' -out ', file_name_pdf]);
system(['rm ', file_name_eps]);



% SGEMM

printf('sgemm\n');

f1 = figure();

sgemm_nn
save -mat sgemm_nn.mat A B

plot(B(:,1), B(:,2));

sgemm_nt
save -mat sgemm_nt.mat A B

hold all
plot(B(:,1), B(:,2));
hold off

sgemm_tn
save -mat sgemm_tn.mat A B

hold all
plot(B(:,1), B(:,2));
hold off

sgemm_tt
save -mat sgemm_tt.mat A B

hold all
plot(B(:,1), B(:,2));
hold off

Gflops_max = A(1)*A(2);

axis([0 300 0 Gflops_max]);
legend('sgemm\_nn', 'sgemm\_nt', 'sgemm\_tn', 'sgemm\_tt', 'Location', 'SouthEast');
xlabel('matrix size n')
ylabel('Gflops')
grid on
ytick = get(gca, 'ytick');
ytick = [ytick, Gflops_max];
set(gca, 'ytick', ytick);

file_name = ['sgemm'];
file_name_eps = [file_name, '.eps'];
file_name_pdf = [file_name, '.pdf'];
print(f1, file_name_eps, '-depsc') 
system(['epstopdf ', file_name_eps, ' -out ', file_name_pdf]);
system(['rm ', file_name_eps]);



% SSYRK

printf('ssyrk\n');

f1 = figure();

ssyrk_ln
save -mat ssyrk_ln.mat A B

plot(B(:,1), B(:,2));

Gflops_max = A(1)*A(2);

axis([0 300 0 Gflops_max]);
legend('ssyrk\_ln', 'Location', 'SouthEast');
xlabel('matrix size n')
ylabel('Gflops')
grid on
ytick = get(gca, 'ytick');
ytick = [ytick, Gflops_max];
set(gca, 'ytick', ytick);

file_name = ['ssyrk'];
file_name_eps = [file_name, '.eps'];
file_name_pdf = [file_name, '.pdf'];
print(f1, file_name_eps, '-depsc') 
system(['epstopdf ', file_name_eps, ' -out ', file_name_pdf]);
system(['rm ', file_name_eps]);



% STRMM

printf('strmm\n');

f1 = figure();

strmm_rlnn
save -mat strmm_rlnn.mat A B

plot(B(:,1), B(:,2));

strmm_rutn
save -mat strmm_rutn.mat A B

hold all
plot(B(:,1), B(:,2));
hold off

Gflops_max = A(1)*A(2);

axis([0 300 0 Gflops_max]);
legend('strmm\_rlnn', 'strmm\_rutn', 'Location', 'SouthEast');
xlabel('matrix size n')
ylabel('Gflops')
grid on
ytick = get(gca, 'ytick');
ytick = [ytick, Gflops_max];
set(gca, 'ytick', ytick);

file_name = ['strmm'];
file_name_eps = [file_name, '.eps'];
file_name_pdf = [file_name, '.pdf'];
print(f1, file_name_eps, '-depsc') 
system(['epstopdf ', file_name_eps, ' -out ', file_name_pdf]);
system(['rm ', file_name_eps]);



% STRSM

printf('strsm\n');

f1 = figure();

strsm_llnu
save -mat strsm_llnu.mat A B

plot(B(:,1), B(:,2));

strsm_lunn
save -mat strsm_lunn.mat A B

hold all
plot(B(:,1), B(:,2));
hold off

strsm_rltn
save -mat strsm_rltn.mat A B

hold all
plot(B(:,1), B(:,2));
hold off

strsm_rltu
save -mat strsm_rltu.mat A B

hold all
plot(B(:,1), B(:,2));
hold off

strsm_rutn
save -mat strsm_rutn.mat A B

hold all
plot(B(:,1), B(:,2));
hold off

Gflops_max = A(1)*A(2);

axis([0 300 0 Gflops_max]);
legend('strsm\_llnu', 'strsm\_lunn', 'strsm\_rltn', 'strsm\_rltu', 'strsm\_rutn', 'Location', 'SouthEast');
xlabel('matrix size n')
ylabel('Gflops')
grid on
ytick = get(gca, 'ytick');
ytick = [ytick, Gflops_max];
set(gca, 'ytick', ytick);

file_name = ['strsm'];
file_name_eps = [file_name, '.eps'];
file_name_pdf = [file_name, '.pdf'];
print(f1, file_name_eps, '-depsc') 
system(['epstopdf ', file_name_eps, ' -out ', file_name_pdf]);
system(['rm ', file_name_eps]);



% SGEQRF

printf('sgeqrf\n');

f1 = figure();

sgelqf
save -mat sgelqf.mat A B

plot(B(:,1), B(:,2));

sgeqrf
save -mat sgeqrf.mat A B

hold all
plot(B(:,1), B(:,2));
hold off

Gflops_max = A(1)*A(2);

axis([0 300 0 Gflops_max]);
legend('sgelqf', 'sgeqrf', 'Location', 'SouthEast');
xlabel('matrix size n')
ylabel('Gflops')
grid on
ytick = get(gca, 'ytick');
ytick = [ytick, Gflops_max];
set(gca, 'ytick', ytick);

file_name = ['sgeqrf'];
file_name_eps = [file_name, '.eps'];
file_name_pdf = [file_name, '.pdf'];
print(f1, file_name_eps, '-depsc') 
system(['epstopdf ', file_name_eps, ' -out ', file_name_pdf]);
system(['rm ', file_name_eps]);



% SGETRF

printf('sgetrf\n');

f1 = figure();

sgetrf_nopivot
save -mat sgetrf_nopivot.mat A B

plot(B(:,1), B(:,2));

sgetrf_rowpivot
save -mat sgetrf_rowpivot.mat A B

hold all
plot(B(:,1), B(:,2));
hold off

Gflops_max = A(1)*A(2);

axis([0 300 0 Gflops_max]);
legend('sgetrf\_nopivot', 'sgetrf\_rowpivot', 'Location', 'SouthEast');
xlabel('matrix size n')
ylabel('Gflops')
grid on
ytick = get(gca, 'ytick');
ytick = [ytick, Gflops_max];
set(gca, 'ytick', ytick);

file_name = ['sgetrf'];
file_name_eps = [file_name, '.eps'];
file_name_pdf = [file_name, '.pdf'];
print(f1, file_name_eps, '-depsc') 
system(['epstopdf ', file_name_eps, ' -out ', file_name_pdf]);
system(['rm ', file_name_eps]);



% SPOTRF

printf('spotrf\n');

f1 = figure();

spotrf_l
save -mat spotrf_l.mat A B

plot(B(:,1), B(:,2));

spotrf_u
save -mat spotrf_u.mat A B

hold all
plot(B(:,1), B(:,2));
hold off

Gflops_max = A(1)*A(2);

axis([0 300 0 Gflops_max]);
legend('spotrf\_l', 'spotrf\_u', 'Location', 'SouthEast');
xlabel('matrix size n')
ylabel('Gflops')
grid on
ytick = get(gca, 'ytick');
ytick = [ytick, Gflops_max];
set(gca, 'ytick', ytick);

file_name = ['spotrf'];
file_name_eps = [file_name, '.eps'];
file_name_pdf = [file_name, '.pdf'];
print(f1, file_name_eps, '-depsc') 
system(['epstopdf ', file_name_eps, ' -out ', file_name_pdf]);
system(['rm ', file_name_eps]);



% SGEMV

printf('sgemv\n');

f1 = figure();

sgemv_n
save -mat sgemv_n.mat A B

plot(B(:,1), B(:,2));

sgemv_t
save -mat sgemv_t.mat A B

hold all
plot(B(:,1), B(:,2));
hold off

sgemv_nt
save -mat sgemv_nt.mat A B

hold all
plot(B(:,1), B(:,2));
hold off

Gflops_max = A(1)*A(2);

axis([0 300 0 Gflops_max]);
legend('sgemv\_n', 'sgemv\_t', 'sgemv\_nt', 'Location', 'SouthEast');
xlabel('matrix size n')
ylabel('Gflops')
grid on
ytick = get(gca, 'ytick');
ytick = [ytick, Gflops_max];
set(gca, 'ytick', ytick);

file_name = ['sgemv'];
file_name_eps = [file_name, '.eps'];
file_name_pdf = [file_name, '.pdf'];
print(f1, file_name_eps, '-depsc') 
system(['epstopdf ', file_name_eps, ' -out ', file_name_pdf]);
system(['rm ', file_name_eps]);



% STRMV

printf('strmv\n');

f1 = figure();

strmv_lnn
save -mat strmv_lnn.mat A B

plot(B(:,1), B(:,2));

strmv_ltn
save -mat strmv_ltn.mat A B

hold all
plot(B(:,1), B(:,2));
hold off

Gflops_max = A(1)*A(2);

axis([0 300 0 Gflops_max]);
legend('strmv\_lnn', 'strmv\_ltn', 'Location', 'SouthEast');
xlabel('matrix size n')
ylabel('Gflops')
grid on
ytick = get(gca, 'ytick');
ytick = [ytick, Gflops_max];
set(gca, 'ytick', ytick);

file_name = ['strmv'];
file_name_eps = [file_name, '.eps'];
file_name_pdf = [file_name, '.pdf'];
print(f1, file_name_eps, '-depsc') 
system(['epstopdf ', file_name_eps, ' -out ', file_name_pdf]);
system(['rm ', file_name_eps]);



% STRSV

printf('strsv\n');

f1 = figure();

strsv_lnn
save -mat strsv_lnn.mat A B

plot(B(:,1), B(:,2));

strsv_ltn
save -mat strsv_ltn.mat A B

hold all
plot(B(:,1), B(:,2));
hold off

Gflops_max = A(1)*A(2);

axis([0 300 0 Gflops_max]);
legend('strsv\_lnn', 'strsv\_ltn', 'Location', 'SouthEast');
xlabel('matrix size n')
ylabel('Gflops')
grid on
ytick = get(gca, 'ytick');
ytick = [ytick, Gflops_max];
set(gca, 'ytick', ytick);

file_name = ['strsv'];
file_name_eps = [file_name, '.eps'];
file_name_pdf = [file_name, '.pdf'];
print(f1, file_name_eps, '-depsc') 
system(['epstopdf ', file_name_eps, ' -out ', file_name_pdf]);
system(['rm ', file_name_eps]);



% SSYMV

printf('ssymv\n');

f1 = figure();

ssymv_l
save -mat ssymv_l.mat A B

plot(B(:,1), B(:,2));

Gflops_max = A(1)*A(2);

axis([0 300 0 Gflops_max]);
legend('ssymv\_l', 'Location', 'SouthEast');
xlabel('matrix size n')
ylabel('Gflops')
grid on
ytick = get(gca, 'ytick');
ytick = [ytick, Gflops_max];
set(gca, 'ytick', ytick);

file_name = ['ssymv'];
file_name_eps = [file_name, '.eps'];
file_name_pdf = [file_name, '.pdf'];
print(f1, file_name_eps, '-depsc') 
system(['epstopdf ', file_name_eps, ' -out ', file_name_pdf]);
system(['rm ', file_name_eps]);

