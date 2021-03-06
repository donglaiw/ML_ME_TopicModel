function plot_pdf(h,name,xs,xn,yn,l,tn,ys)

xlabel(xn,'fontsize',25,'FontName','Times','FontWeight','bold')
if nargin >=5 &&length(yn)>0
ylabel(yn,'fontsize',25,'FontName','Times','FontWeight','bold')
end
axis square
axis tight
set(gca,'XTick',xs,'fontsize',15,'FontName','Times','FontWeight','bold')
if nargin>=6 &&length(l)>0
le=legend([l(:)]);
leg = findobj(le,'type','text');
set(leg,'fontsize',20,'FontName','Times','FontWeight','bold')
pause
end

if nargin >=7&&length(tn)>0
title(tn,'fontsize',25,'FontName','Times','FontWeight','bold')
end

if nargin ==8&&length(ys)>0
axis([min(xs) max(xs) min(ys) max(ys)])
end

save_pdf(h,name);
close



