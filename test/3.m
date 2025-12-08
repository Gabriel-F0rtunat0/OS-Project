# PASSO: comando de espaçamento de movimentos
# Só existe um no início por ficheiro. Indica quantas jogadas esperar
# entre cada movimento.
PASSO 1
# POS: comando de colocação inicial do monstro (linha e coluna).
# Assume-se que não é possível o monstro ser colocado 
# numa posição impossível/inexistente.
# Só existe um no início por ficheiro.
POS 3 10
# Todos os comandos após PASSO e POS são executados em ciclo infinito.
# Comandos: W (cima), A (esquerda), S (baixo), D (direita),
# R (direção aleatória), T n (espera n jogadas), C (carregar).
# Este padrão mistura movimentos ortogonais, cargas e esperas.
C
W
W
T 1
D
D
S
S
C
A
A
T 3
R
R
W
D
S
A
