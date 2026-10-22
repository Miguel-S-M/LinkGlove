# Sobe a versão do site (VERSION no sw.js e ?v= no index.html, js/imagens.js e sw.js).
# Use SEMPRE depois de mudar qualquer arquivo, antes de publicar:  python subir-versao.py
import os, re
d = os.path.dirname(os.path.abspath(__file__))
sw = open(os.path.join(d, 'sw.js'), encoding='utf-8').read()
n = int(re.search(r"VERSION = '(\d+)'", sw).group(1)); m = n + 1
for f in ['sw.js', 'index.html', os.path.join('js', 'imagens.js')]:
    p = os.path.join(d, f); s = open(p, encoding='utf-8').read()
    s = re.sub(r'\?v=%d\b' % n, '?v=%d' % m, s)
    if f == 'sw.js': s = s.replace("VERSION = '%d'" % n, "VERSION = '%d'" % m)
    open(p, 'w', encoding='utf-8').write(s)
print('Versão %d -> %d. Agora publique todos os arquivos.' % (n, m))
