% assumes existence of a table named all
n = 15;

% 12 tasks
tasks = 12;

ttf = {};
ratio = {};
for k = 1:12
  ttf{k} = zeros(n, 3);
  ratio{k} = zeros(n, 3);
end

% we have n subjects
for k = 1:length(all)
  table = all{k, 2};
  row = all{k, 1};
  col = 0;
  
  if (strcmp(all{k, 3},'none'))
    col = 1;
  elseif (strcmp(all{k, 3}, 'alpha'))
    col = 2;
  elseif (strcmp(all{k, 3}, 'animation'))
    col = 3;
  end
  
  ttf{table}(row, col) = all{k, 7};
  ratio{table}(row, col) = all{k, 14};
end

mwu_ttf_alpha = zeros(12,1);
mwu_ttf_anim = zeros(12,1);
mwu_ratio_alpha = zeros(12,1);
mwu_ratio_anim = zeros(12,1);
for k = 1:12
  mwu_ttf_alpha(k) = ranksum(ttf{k}(:,1), ttf{k}(:,2));
  mwu_ttf_anim(k) = ranksum(ttf{k}(:,1), ttf{k}(:,3));
  mwu_ratio_alpha(k) = ranksum(ratio{k}(:,1), ratio{k}(:,2));
  mwu_ratio_anim(k) = ranksum(ratio{k}(:,1), ratio{k}(:,3));
end