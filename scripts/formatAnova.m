% assumes existence of table consisting of results for single task
% table is called a for now

n = 15;

% we have n subjects
anTable = table('Size', [n 3], 'VariableTypes', {'double','double','double'}, 'VariableNames', {'none', 'alpha', 'animation'});

for k = 1:length(a)
  row = a{k};
  col = 0;
  
  if (strcmp(a(k, 3),'none'))
    col = 1;
  elseif (strcmp(a{k, 3}, 'alpha'))
    col = 2;
  elseif (strcmp(a{k, 3}, 'animation'))
    col = 3;
  end
  
  anTable(row, col) = { log(a{k, 14}) };
end