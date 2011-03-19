"""Convert Swish NEWS file to HTML.
"""

_NEWS = '../NEWS'
_VERSIONED_NEWS = '../NEWS-%s.html'

def main():

    with open(_NEWS, 'r') as news_in:
        html = ('''<html style="font-size:x-small; font-family : sans-serif">
<body>
%s
</body>
</html>''') % read_news(news_in, 1)

    with open(_NEWS, 'r') as news_in:
        version = extract_version(news_in)
    with open(_VERSIONED_NEWS % version, 'w') as html_open:
        html_open.write(html)

def extract_version(news):
    return news.readline().strip('\n\r').replace('swish-', '')

def read_news(news, indent):
    entry_html = '''<h1 style="font-size:small; margin-bottom:0ex; color:brown">%s</h1>
<p style="margin-top:0ex">
<ul style="margin-left:3ex; margin-top:1ex">
%s
</ul>
</p>'''
    html = []
    while True:
        entry = read_entry(news)
        if not entry:
            break
        lines = []
        for item in entry['items']:
            lines.append(create_list_item(item, indent + 1))
        html.append(entry_html % (entry['heading'], '\n'.join(lines)))

    return '\n'.join(html)

def create_list_item(item, indent):
    return '%s<li>%s</li>' % (('\t' * indent), item)

def read_entry(news):
    entry = { 'heading' : news.readline().strip(), 'items' : [] }

    while True:
        line = news.readline().strip('\n\r')
        print line
        if not line:
            break
        if line[0] == ' ':
            entry['items'][-1] += line
            continue
        if line[0] != '-':
            raise Exception('Incorrect format: %s' % line)
        entry['items'].append(line[1:])

    if not entry['heading']:
        return None

    return entry

if __name__ == '__main__':
    main()

