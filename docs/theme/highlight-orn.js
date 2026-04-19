hljs.registerLanguage('orn', function (hljs) {
  return {
    name: 'Orn',
    keywords: {
      keyword:
        'fn let const struct enum impl type match if elif else ' +
        'while for in ret break continue defer import syscall ' +
        'sizeof as null true false',
      type: 'int unsigned float double bool void char string',
      literal: 'true false null',
    },
    contains: [
      hljs.C_LINE_COMMENT_MODE,
      hljs.C_BLOCK_COMMENT_MODE,
      hljs.QUOTE_STRING_MODE,
      {
        className: 'string',
        begin: "'",
        end: "'",
        contains: [hljs.BACKSLASH_ESCAPE],
      },
      hljs.C_NUMBER_MODE,
      {
        className: 'number',
        begin: '\\b0[xXoObB][0-9a-fA-F_]+\\b',
      },
      {
        className: 'function',
        beginKeywords: 'fn',
        end: '\\{',
        excludeEnd: true,
        contains: [
          hljs.UNDERSCORE_TITLE_MODE,
          {
            className: 'params',
            begin: '\\(',
            end: '\\)',
            contains: [
              {
                className: 'type',
                begin: '\\*[A-Za-z_][A-Za-z0-9_]*',
              },
            ],
            keywords: {
              type: 'int unsigned float double bool void char string',
            },
          },
        ],
      },
      {
        className: 'title',
        beginKeywords: 'struct enum impl type',
        end: '[\\{=]',
        excludeEnd: true,
        contains: [hljs.UNDERSCORE_TITLE_MODE],
      },
    ],
  };
});

document.querySelectorAll('code.language-orn').forEach(function (block) {
  block.classList.remove('hljs');
  block.removeAttribute('data-highlighted');
  block.innerHTML = block.textContent;

  if (hljs.highlightElement) {
    hljs.highlightElement(block);
  } else {
    hljs.highlightBlock(block);
  }
});
