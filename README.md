# Controle de Estoque com Matriz de LEDs e Botões

## Descrição do Projeto
Este projeto implementa um sistema embarcado para controle de estoque utilizando uma matriz de LEDs para exibição numérica, botões para interação do usuário e um LED RGB para indicação de diferentes estados. O objetivo é oferecer uma solução simples e de baixo custo para monitoramento manual de pequenos estoques.

## Funcionalidades
- Exibição numérica da quantidade de itens em estoque utilizando uma matriz de LEDs.
- Alternância entre diferentes prateleiras com o botão A.
- Adição de itens ao estoque com o botão B.
- Indicação da prateleira selecionada por meio do LED RGB.

## Componentes Utilizados
- Microcontrolador Raspberry Pi Pico
- Matriz de LEDs WS2812
- LED RGB
- Botões de entrada

## Esquema de Conexões
| Pino | Componente   | Função                     |
|------|-------------|----------------------------|
| 7    | WS2812 LED  | Controle da matriz de LEDs |
| 5    | Botão A     | Alternar prateleiras       |
| 6    | Botão B     | Adicionar item             |
| 8    | LED RGB     | Indicação da prateleira    |

## Configuração e Instalação
1. Instale o ambiente de desenvolvimento para Raspberry Pi Pico.
2. Clone este repositório.
3. Compile e carregue o código no microcontrolador.
4. Conecte os componentes de acordo com o esquema de conexões.

## Execução
Após a inicialização:
- Pressione o botão A para alternar entre prateleiras.
- Pressione o botão B para adicionar um item à prateleira selecionada.
- O número de itens será exibido na matriz de LEDs.
- O LED RGB indicará a prateleira ativa.

## Testes e Validação
Os testes consistem em verificar:
- Resposta dos botões de controle.
- Atualização correta da matriz de LEDs.
- Mudança da cor do LED RGB conforme esperado.

## YUTUBE
https://youtu.be/pNBmzJY9DJ0
